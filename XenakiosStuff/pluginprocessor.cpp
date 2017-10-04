#define WDL_NO_DEFINE_MINMAX

#include "pluginprocessor.h"
#include "../reaper plugin/reaper_plugin_functions.h"
#include <thread>
#include <mutex>
#include <algorithm>
#ifdef WIN32
#include <ppl.h>
#endif

AudioPluginFormatManager* g_plugformat_manager = nullptr;
extern std::unique_ptr<PropertiesFile> g_properties_file;


inline void showConsoleMsg(const String& str)
{
	ShowConsoleMsg(str.toRawUTF8());
	ShowConsoleMsg("\n");
}

PluginChain::PluginChain()
{
	if (g_plugformat_manager == nullptr)
	{
		g_plugformat_manager = new AudioPluginFormatManager;
		g_plugformat_manager->addDefaultFormats();
	}
}

PluginChain::~PluginChain()
{
	for (auto& e : m_plugins)
	{
		delete e.m_plug->getActiveEditor();
	}
}

bool PluginChain::addPlugin(PluginDescription & plugdesc)
{
	String err;
	AudioPluginInstance* plug = g_plugformat_manager->createPluginInstance(plugdesc, 44100.0, 512, err);
	if (plug != nullptr)
	{
		m_plugins.emplace_back(std::shared_ptr<AudioPluginInstance>(plug));
		return true;
	}
	else
	{
		showConsoleMsg(err);
	}
	return false;
}

void PluginChain::removeAllPlugins()
{
	for (auto& e : m_plugins)
		delete e.m_plug->getActiveEditor();
	m_plugins.clear();
}

void PluginChain::removePlugin(int index)
{
	if (index >= 0 && index < m_plugins.size())
	{
		auto ed = m_plugins[index].m_plug->getActiveEditor();
		if (ed != nullptr)
		{
			delete ed;
		}
		m_plugins.erase(m_plugins.begin() + index);
	}
}

AudioPluginInstance * PluginChain::getPlugin(int index)
{
	if (index >= 0 && index < m_plugins.size())
		return m_plugins[index].m_plug.get();
	return nullptr;
}

void PluginChain::render(MediaItem * item, double sr, String outfn)
{
	if (item == nullptr || m_plugins.size()==0)
		return;
	MediaItem_Take* take = GetActiveTake(item);
	AudioAccessor* accessor = CreateTakeAudioAccessor(take);
	if (accessor != nullptr)
	{
		double start = GetAudioAccessorStartTime(accessor);
		double end = GetAudioAccessorEndTime(accessor);
		double len = end - start;
		int64_t lenframes = len * sr;
		int outchans = 2;
		char cfg[] = { 'e','v','a','w', 32, 0 };
		std::unique_ptr<PCM_sink> sink;
		sink = std::unique_ptr<PCM_sink>(PCM_Sink_Create(outfn.toRawUTF8(),
			cfg, sizeof(cfg), outchans, sr, true));
		if (sink != nullptr)
		{
			int bufsize = 512;
			std::vector<double> buf(bufsize*outchans);
			MidiBuffer midibuf;
			AudioBuffer<float> plugprocbuf(8,bufsize);
			float** plugbufptrs = plugprocbuf.getArrayOfWritePointers();
			int64_t count = 0;
			std::vector<double> diskoutbuf(outchans*bufsize);
			double* diskoutbufptrs[64];
			for (int i = 0; i < outchans; ++i)
				diskoutbufptrs[i] = &diskoutbuf[i*bufsize];
			//m_plugins[0]->setProcessingPrecision(AudioProcessor::doublePrecision);
			for (auto& e : m_plugins)
			{
				e.m_plug->reset();
				e.m_plug->setPlayConfigDetails(outchans, outchans, sr, bufsize);
				e.m_plug->prepareToPlay(sr, bufsize);
			}
			while (count < lenframes)
			{
				GetAudioAccessorSamples(accessor, sr, outchans, (double)count/sr, bufsize, buf.data());
				for (int i = 0; i < outchans; ++i)
				{
					for (int j = 0; j < bufsize; ++j)
					{
						plugbufptrs[i][j] = buf[j*outchans + i];
					}
				}
				for (auto& e : m_plugins)
					e.m_plug->processBlock(plugprocbuf, midibuf);
				for (int i = 0; i < outchans; ++i)
				{
					for (int j = 0; j < bufsize; ++j)
					{
						diskoutbufptrs[i][j] = plugbufptrs[i][j];
					}
				}
				sink->WriteDoubles(diskoutbufptrs, bufsize, outchans, 0, 1);
				count += bufsize;
			}
		}
		
		DestroyAudioAccessor(accessor);
		for (auto& e : m_plugins)
			e.m_plug->releaseResources();
	}
	
}

void PluginChain::render(std::vector<std::vector<double>>& buf, double sr, int blocksize, bool* cancel_flag, 
	double* progress)
{
	/* Why is all this fiddling with the smaller processing buffers, sample type conversions etc needed?
	 
	 -While the VST standard technically does allow processing with hours of long buffers etc, in practice
	 we can guess that won't work with some plugins, so the processing is done in smaller blocks. Obviously if automated
	 parameters are wanted at some point, those will require the smaller processing buffers too.
	 
	 -There are still plenty of plugins that report they don't support 64 bit precision processing, so we need to process with
	 32 bit float buffers. (JUCE asserts if 64 bit processing is attempted with such plugins.) If 64 bit floats support is 
	 absolutely required, a separate version of the rendering code can be written that skips the conversion down to 
	 32 bit floats and back to 64 bit floats.

	 -Can implement processing cancellation, which couldn't be done if the plugin is given the whole input buffer to process
	 at once. 
	*/
	int outchans = buf.size();
	int total_latency = 0;
	for (auto& e : m_plugins)
	{
		e.m_plug->reset();
		e.m_plug->setPlayConfigDetails(outchans, outchans, sr, blocksize);
		e.m_plug->prepareToPlay(sr, blocksize);
		// Obviously relies on the plugin updating the latency synchronously, probably won't happen with all plugins
		// after reset and prepareToPlay have been called...But such is life.
		total_latency += e.m_plug->getLatencySamples();
	}
	total_latency = jlimit(0, 500000, total_latency);
	if (total_latency < 0)
		total_latency = 0;
	int64_t inposcount = 0;
	int64_t lenframes = buf[0].size()+total_latency;
	int64_t inputlenframes = buf[0].size();
	MidiBuffer midibuf;
	AudioBuffer<float> plugprocbuf(outchans, blocksize);
	float** plugbufptrs = plugprocbuf.getArrayOfWritePointers();
	while (inposcount < lenframes)
	{
		if (cancel_flag != nullptr && *cancel_flag == true)
			break;
		int framesto_output = std::min<int64_t>(blocksize, lenframes - inposcount);
		for (int i = 0; i < outchans; ++i)
		{
			for (int j = 0; j < blocksize; ++j)
			{
				if (j<framesto_output && inposcount+j<inputlenframes)
					plugbufptrs[i][j] = buf[i][inposcount+j];
				else plugbufptrs[i][j] = 0.0;
			}
		}
		for (auto& e : m_plugins)
			e.m_plug->processBlock(plugprocbuf, midibuf);
		for (int i = 0; i < outchans; ++i)
		{
			for (int j = 0; j < framesto_output; ++j)
			{
				if (inposcount + j >= total_latency)
				{
					buf[i][inposcount + j - total_latency] = plugbufptrs[i][j];
				}
			}
		}
		inposcount += blocksize;
		if (progress != nullptr)
		{
			*progress = 1.0/lenframes*inposcount;
		}
	}
	//Logger::writeToLog(String(foo));
	for (auto& e : m_plugins)
		e.m_plug->releaseResources();
}

PluginChain* PluginChain::duplicate()
{
	PluginChain* dupl = new PluginChain();
	for (auto& e : m_plugins)
	{
		String err;
		AudioPluginInstance* plug = g_plugformat_manager->createPluginInstance(e.m_plug->getPluginDescription(), 44100.0, 512, err);
		if (plug != nullptr)
		{
			MemoryBlock state;
			e.m_plug->getStateInformation(state);
			plug->setStateInformation(state.getData(), state.getSize());
			dupl->m_plugins.emplace_back(std::shared_ptr<AudioPluginInstance>(plug));
		}
	}
	return dupl;
}

void PluginChain::setThumbImage(int index, Image img)
{
	if (index >= 0 && index < m_plugins.size())
	{
		m_plugins[index].m_thumb = img;
	}
}

ValueTree PluginChain::getState()
{
	ValueTree result("chainstate");
	for (auto& e : m_plugins)
	{
		ValueTree plugstate("plugstate");
		PluginDescription desc = e.m_plug->getPluginDescription();
		plugstate.setProperty("plugname", desc.name, nullptr);
		plugstate.setProperty("plugfileorid", desc.fileOrIdentifier, nullptr);
		plugstate.setProperty("plugfmt", desc.pluginFormatName, nullptr);
		plugstate.setProperty("pluguid", desc.uid, nullptr);
		MemoryBlock block;
		e.m_plug->getStateInformation(block);
		plugstate.setProperty("chunk", block, nullptr);
		result.addChild(plugstate, -1, nullptr);
	}
	return result;
}

void PluginChain::setState(ValueTree state)
{
	if (state.isValid() == false)
		return;
	m_plugins.clear();
	int numchildren = state.getNumChildren();
	for (int i = 0; i < numchildren; ++i)
	{
		ValueTree plugstate = state.getChild(i);
		String plugname = plugstate.getProperty("plugname");
		String plugid = plugstate.getProperty("plugfileorid");
		String plugfmt = plugstate.getProperty("plugfmt");
		int pluguid = plugstate.getProperty("pluguid");
		PluginDescription desc;
		desc.pluginFormatName = plugfmt;
		desc.fileOrIdentifier = plugid;
		//desc.uid = pluguid;
		desc.name = plugname;
		//showConsoleMsg(plugfmt + " " + plugid);
		if (addPlugin(desc) == true)
		{
			const juce::var& temp = plugstate.getProperty("chunk");
			MemoryBlock* block = temp.getBinaryData();
			if (block!=nullptr)
				m_plugins.back().m_plug->setStateInformation(block->getData(), block->getSize());
		}
		
	}
}

void PluginChain::shutDown()
{
	delete g_plugformat_manager;
	g_plugformat_manager = nullptr;
	// Plugin hosting apparently creates the MM instance, so need to do this to avoid memory leak spam from Juce when debugging
	MessageManager::deleteInstance();
}

std::shared_ptr<PluginChain> PluginChain::createFromFile(String fn)
{
	File file(fn);
	FileInputStream* instream = file.createInputStream();
	if (instream != nullptr)
	{
		ValueTree state = ValueTree::readFromStream(*instream);
		auto result = std::make_shared<PluginChain>();
		result->setState(state);
		delete instream;
		return result;
	}
	return std::shared_ptr<PluginChain>();
}

PluginProcessingWindow::PluginProcessingWindow(String title, int w, int h, bool resizable, Colour bgcolor) :
	MyWindow(title, w, h, resizable, bgcolor)
{
	m_content = new PluginProcessingContent;
	setContentOwned(m_content, false);
}

void testPluginChain()
{
	make_window<PluginProcessingWindow>("Plugin processor", 500, 130, true, Colours::grey);
}

PluginProcessingContent::PluginProcessingContent()
{
	m_chain = std::make_shared<PluginChain>();
	m_chain_editor = std::make_shared<PluginChainEditor>(m_chain.get());
    char ppbuf[4096] = {0};
    GetProjectPath(ppbuf,4096);
    m_outfile_edit.setText(String(CharPointer_UTF8(ppbuf))+"/test", false);
	addAndMakeVisible(&m_outfile_edit);
	addAndMakeVisible(&m_render_but);
	m_render_but.setButtonText("Render");
	m_render_but.addListener(this);
	m_render_but.setEnabled(false);
	addAndMakeVisible(&m_plugin_choose_but);
	m_plugin_choose_but.setButtonText("Add plugin...");
	m_plugin_choose_but.addListener(this);
	addAndMakeVisible(&m_plugin_manager_but);
	m_plugin_manager_but.setButtonText("Manage plugins...");
	m_plugin_manager_but.addListener(this);
	m_file_but.setButtonText("Save/Load");
	m_file_but.addListener(this);
	addAndMakeVisible(&m_file_but);
	
	addAndMakeVisible(m_chain_editor.get());
	initPlugList();
	m_chain_editor->PluginSelectedCallback = [this](int index)
	{
		pluginSelected(index);
	};
	m_chain_editor->PluginAboutToBeDeletedCallback = [this](int index)
	{
		m_gen_ed = nullptr;
		m_currently_visible_editor = -1;
	};
}

PluginProcessingContent::~PluginProcessingContent()
{
	m_gen_ed = nullptr;
	m_chain->removeAllPlugins();
}

void PluginProcessingContent::resized()
{
	int x = 1;
	m_outfile_edit.setBounds(1, 1, getWidth() - 120, 25);
	m_render_but.setBounds(x, 30, 60, 25);
	x += 65;
	m_plugin_choose_but.setBounds(x, 30, 100, 25);
	x += 105;
	m_plugin_manager_but.setBounds(x, 30, 120, 25);
	x += 125;
	m_file_but.setBounds(x, 30, 120, 25);
	m_chain_editor->setBounds(1, 60, getWidth() - 2, 55);
	if (m_gen_ed != nullptr)
		m_gen_ed->setBounds(1, 120, getWidth() - 2, m_gen_ed->getHeight());
	if (m_currently_visible_editor >= 0)
	{
		auto pluged = m_chain->getPlugin(m_currently_visible_editor)->getActiveEditor();
		if (pluged!=nullptr)
			pluged->setBounds(1, 120, getWidth() - 2, pluged->getHeight());
	}
}

void PluginProcessingContent::buttonClicked(Button * b)
{
	if (b == &m_file_but)
	{
		PopupMenu menu;
		menu.addItem(1, "Save chain to file...", true, false);
		menu.addItem(2, "Load chain from file...", true, false);
		int r = menu.show();
		if (r == 1)
		{
			saveChainToFile();
		}
		if (r == 2)
		{
			loadChainFromFile();
		}
	}
	if (b == &m_render_but)
	{
		renderSelectedItems();
	}
	if (b == &m_plugin_choose_but && m_pluginlist.getNumTypes()>0)
	{
		PopupMenu menu;
		int id = 1;
		for (auto& e : m_pluginlist)
		{
			menu.addItem(id, e->name, true, false);
			++id;
		}
		int r = menu.show();
		if (r > 0)
		{
			m_chosen_plugin_type = r - 1;
			//m_chain.removeAllPlugins();
			m_chain->addPlugin(*m_pluginlist.getType(m_chosen_plugin_type));
			m_chain_editor->repaint();
			m_render_but.setEnabled(true);
			pluginSelected(m_chain->numPlugins() - 1);
			auto plug = m_chain->getPlugin(0);
			if (plug != nullptr)
			{
				/*
				if (plug->hasEditor() == false)
				{
					GenericAudioProcessorEditor* ed = new GenericAudioProcessorEditor(plug);
					m_plugin_editor = std::unique_ptr<GenericAudioProcessorEditor>(ed);
				}
				else
				{
					m_plugin_editor = std::unique_ptr<AudioProcessorEditor>(plug->createEditor());
				}
				addAndMakeVisible(m_plugin_editor.get());
				getParentComponent()->setSize(getParentComponent()->getWidth(), m_plugin_editor->getHeight() + 70);
				*/
			}
		}
	}
	if (b == &m_plugin_manager_but)
	{
		juce::AlertWindow aw("Manage plugins", "", AlertWindow::NoIcon, this);
		aw.addButton("OK", 1);
		aw.addButton("Cancel", 2);
		AudioPluginFormatManager plugmanager;
		plugmanager.addDefaultFormats();
		PluginListComponent* pluglistcomp = new PluginListComponent(plugmanager, m_pluginlist, File(), g_properties_file.get());
		pluglistcomp->setBounds(0, 0, 800, 600);
		aw.addCustomComponent(pluglistcomp);
		int r = aw.runModalLoop();
		if (r == 1)
		{
			XmlElement* xml = m_pluginlist.createXml();
			if (xml != nullptr)
			{
				g_properties_file->setValue("plugin_scan_results", xml);
				delete xml;
			}
		}
		delete pluglistcomp;
	}
}

void PluginProcessingContent::initPlugList()
{
	XmlElement* elem = g_properties_file->getXmlValue("plugin_scan_results");
	if (elem != nullptr)
	{
		m_pluginlist.recreateFromXml(*elem);
        m_pluginlist.sort(KnownPluginList::sortAlphabetically, true);
        delete elem;
	}
}

void PluginProcessingContent::pluginSelected(int index)
{
	if (index >= 0 && index!=m_currently_visible_editor)
	{
		if (m_chain->getPlugin(index)->hasEditor() == false)
		{
			if (m_currently_visible_editor >= 0)
				delete m_chain->getPlugin(m_currently_visible_editor)->getActiveEditor();
			m_gen_ed = std::make_unique<GenericAudioProcessorEditor>(m_chain->getPlugin(index));
			addAndMakeVisible(m_gen_ed.get());
			m_currently_visible_editor = index;
			resized();
		}
		else
		{
			m_gen_ed = nullptr;
			if (m_currently_visible_editor >= 0)
				delete m_chain->getPlugin(m_currently_visible_editor)->getActiveEditor();
			AudioProcessorEditor* ed = m_chain->getPlugin(index)->createEditorIfNeeded();
			addAndMakeVisible(ed);
            ed->setTopLeftPosition(1, 120);
            m_currently_visible_editor = index;
			resized();
		}
		
	}
	
}

void PluginProcessingContent::renderSelectedItems()
{
	int numsel = CountSelectedMediaItems(nullptr);
	if (m_chosen_plugin_type >= 0 && numsel>0)
	{
		for (int i = 0; i < numsel; ++i)
		{
			MediaItem* item = GetSelectedMediaItem(nullptr, i);
            String outfn = m_outfile_edit.getText() + "-" + String(Time::currentTimeMillis()) + ".wav";
			m_chain->render(item, 44100.0, outfn);
            bool add_as_new_take = true; //g_properties_file->getBoolValue("pluginrender_addasnewtake");
			if (add_as_new_take == true)
			{
				PCM_source* src = PCM_Source_CreateFromFile(outfn.toRawUTF8());
				if (src != nullptr)
				{
					MediaItem_Take* take = AddTakeToMediaItem(item);
					SetMediaItemTake_Source(take, src);
					SetActiveTake(take);
					UpdateArrange();
				}
			}
		}
	}
}

void PluginProcessingContent::saveChainToFile()
{
	//File initdir = File(g_propsfile->getValue("general/last_sound_import_dir"));
	FileChooser myChooser("Save plugin chain",
		File(),
		"*.pluginchain", true);
	if (myChooser.browseForFileToSave(true) == true)
	{
		File file = myChooser.getResult();
		if (file.existsAsFile())
		{
			file.deleteFile();
		}
		ValueTree state = m_chain->getState();
		FileOutputStream* outstream = file.createOutputStream();
		if (outstream != nullptr)
		{
			state.writeToStream(*outstream);
			delete outstream;
		}
	}
}

void PluginProcessingContent::loadChainFromFile()
{
	FileChooser myChooser("Load plugin chain",
		File(),
		"*.pluginchain", true);
	if (myChooser.browseForFileToOpen() == true)
	{
		File file = myChooser.getResult();
		FileInputStream* instream = file.createInputStream();
		if (instream != nullptr)
		{
			ValueTree state = ValueTree::readFromStream(*instream);
			m_chain->setState(state);
			m_chain_editor->repaint();
			delete instream;
		}
	}
}

PluginChainEditor::PluginChainEditor(PluginChain * chain)
	: m_chain(chain)
{
}

void PluginChainEditor::paint(Graphics & g)
{
	g.fillAll(Colours::black);
	for (int i = 0; i < m_chain->m_plugins.size(); ++i)
	{
		if (m_plug_to_drag == i)
			g.setColour(Colours::cyan);
		else
			g.setColour(Colours::white);
		String txt = m_chain->m_plugins[i].m_plug->getName();
		Image img = m_chain->m_plugins[i].m_thumb;
        int x0 = i*(m_plugin_box_width+20);
        g.drawRect(x0, 0, m_plugin_box_width, 50);
		if (img.isValid() == false)
		{
			g.drawMultiLineText(txt, x0 + 2, 25, m_plugin_box_width-2);
		}
		else
		{
			g.drawImage(img, x0, 1, 78, 48, 0, 0, img.getWidth(), img.getHeight());
		}
		if (i<m_chain->m_plugins.size()-1)
			g.drawArrow(Line<float>(x0 + m_plugin_box_width, 25, x0+m_plugin_box_width+20, 25),2.0f,8.0f,8.0f);
		g.setColour(Colours::green);
		g.fillRect(x0 + 1, 1, 12, 12);
		g.setColour(Colours::black);
		g.drawText(CharPointer_UTF8("X"), x0+1, 1, 12, 12, Justification::centred);
	}
	
}

void PluginChainEditor::mouseDown(const MouseEvent & ev)
{
	int sube = 0;
	m_plug_to_drag = plugIndexFromCoord(ev.x, ev.y, sube);
	if (sube == 1)
	{
		if (m_plug_to_drag>=0)
			PluginAboutToBeDeletedCallback(m_plug_to_drag);
		m_chain->removePlugin(m_plug_to_drag);
		m_plug_to_drag = -1;
	}
	else
		PluginSelectedCallback(m_plug_to_drag);
	repaint();
}

void PluginChainEditor::mouseDrag(const MouseEvent & ev)
{
	int sube = 0;
	int index = plugIndexFromCoord(ev.x, ev.y, sube);
	if (index >= 0 && index < m_chain->m_plugins.size() && m_plug_to_drag>=0)
	{
		std::swap(m_chain->m_plugins[m_plug_to_drag], m_chain->m_plugins[index]);
		m_plug_to_drag = index;
		repaint();
	}
}

void PluginChainEditor::mouseUp(const MouseEvent & ev)
{
	PluginSelectedCallback(m_plug_to_drag);
    m_plug_to_drag = -1;
	repaint();
}

int PluginChainEditor::plugIndexFromCoord(int x, int y, int& subelem)
{
	for (int i = 0; i < m_chain->m_plugins.size(); ++i)
	{
		juce::Rectangle<int> rect(i * (m_plugin_box_width+20), 0, m_plugin_box_width, 50);
		if (rect.contains(x, y))
		{
			rect = juce::Rectangle<int>(i * (m_plugin_box_width+20)+1, 1, 11, 11);
			if (rect.contains(x, y))
				subelem = 1; // close button
			return i;
		}
	}
	return -1;
}

inline std::unique_ptr<PCM_sink> createPCMSink(const String& outfilename, const String& format, char bitdepth, int chans, int samplerate)
{
	if (format == "WAV")
	{
		char cfg[] = { 'e','v','a','w', bitdepth, 0 };
		return std::unique_ptr<PCM_sink>(PCM_Sink_Create(outfilename.toRawUTF8(),
			cfg, sizeof(cfg), chans, samplerate, true));
	}
	return nullptr;
}

String renderFileWithChain(String chainfn, String infn, String outfn)
{
	auto chain = PluginChain::createFromFile(chainfn);
	if (chain != nullptr)
	{
		PCM_source* src = PCM_Source_CreateFromFile(infn.toRawUTF8());
		if (src != nullptr)
		{
			int numoutchans = 2;
			double outsr = 44100.0;
			double tail_len = 0.0;
			int64_t lenframes = outsr*(src->GetLength() + tail_len);
			std::vector<std::vector<double>> procbuf(numoutchans);
			for (auto& e : procbuf)
				e.resize(lenframes);
			std::vector<double> readbuf(lenframes * 2);
			PCM_source_transfer_t transfer = { 0 };
			transfer.length = lenframes;
			transfer.nch = numoutchans;
			transfer.samplerate = outsr;
			transfer.samples = readbuf.data();
			src->GetSamples(&transfer);
			delete src;
			for (int i = 0; i < numoutchans; ++i)
				for (int j = 0; j < lenframes; ++j)
					procbuf[i][j] = readbuf[j * numoutchans + i];
			chain->render(procbuf, outsr);
			auto sink = createPCMSink(outfn, "WAV", 32,
				numoutchans, outsr);
			if (sink != nullptr)
			{
				double* sinkbuf[2] = { procbuf[0].data(), procbuf[1].data() };
				sink->WriteDoubles(sinkbuf, lenframes, numoutchans, 0, 1);
				return String();
			}
			else "could not create sink";
		}
		else
			"Could not create pcm source";

	}
	else return "Could not load plugin chain file";
	return "Unknown error";
}

class PluginChainPool
{
	struct pool_entry
	{
		std::shared_ptr<PluginChain> m_chain;
		bool m_available = true;
	};
	std::vector<pool_entry> m_chains;
	std::mutex m_mutex;
	String m_chainfn;
	int m_pool_misses = 0;
	int m_obtain_try_count = 1000;
public:
	PluginChainPool(String chainfn, int initialchains)
	{
		m_chainfn = chainfn;
		m_chains.resize(initialchains);
		for (int i = 0; i < initialchains; ++i)
		{
			m_chains[i].m_chain = PluginChain::createFromFile(chainfn);
		}
	}
	~PluginChainPool()
	{
		Logger::writeToLog("PluginChainPool had " + String(m_pool_misses) + " misses");
	}
	std::shared_ptr<PluginChain> obtain()
	{
		int sanity = 0;
		while (true)
		{
			for (auto& e : m_chains)
			{
				m_mutex.lock();
				if (e.m_available == true)
				{
					e.m_available = false;
					m_mutex.unlock();
					return e.m_chain;
				}
				m_mutex.unlock();
			}
			Thread::sleep(1);
			++sanity;
			if (sanity == m_obtain_try_count)
				break;
		}
		m_mutex.lock();
		pool_entry entry;
		entry.m_chain = PluginChain::createFromFile(m_chainfn);
		entry.m_available = false;
		m_chains.push_back(entry);
		++m_pool_misses;
		m_mutex.unlock();
		return entry.m_chain;
	}
	void release(std::shared_ptr<PluginChain> c)
	{
		std::lock_guard<std::mutex> locker(m_mutex);
		for (auto& e : m_chains)
			if (e.m_chain == c)
			{
				e.m_available = true;
				return;
			}
	}
};

class FolderRenderComponent : public Component
{
public:
	FolderRenderComponent(int numprogresses)
	{
		m_progresses.resize(numprogresses);
		for (int i = 0; i < numprogresses; ++i)
		{
			auto p = std::make_shared<ProgressBar>(m_progresses[i]);
			addAndMakeVisible(p.get());
			p->setBounds(1, 1+12*i, 500, 11);
			m_bars.push_back(p);
		}
		setSize(502, 1+12* numprogresses);
		setTopLeftPosition(10, 60);
	}
	std::vector<double> m_progresses;
private:
	std::vector<std::shared_ptr<ProgressBar>> m_bars;
};

void renderFolderWithChainMultithreaded(String chainfn, String indir, String outdir, int numthreads)
{
	MessageManager::getInstance();
	DirectoryIterator iter(File(indir), false, "*.wav");
	StringArray filestoprocess;
	while (iter.next())
	{
		filestoprocess.add(iter.getFile().getFullPathName());
	}
	FolderRenderComponent* comp = new FolderRenderComponent(filestoprocess.size());
	comp->addToDesktop(0);
	comp->setVisible(true);
	auto rendertask = [comp, filestoprocess, numthreads, indir, chainfn]()
	{
		
		std::vector<std::vector<std::vector<double>>> buffers(filestoprocess.size());
		double outsr = 44100.0;
		int numoutchans = 2;
		std::vector<double> readbuf;
		for (int i = 0; i < filestoprocess.size(); ++i)
		{
			comp->m_progresses[0] = 1.0 / filestoprocess.size()*i;
			buffers[i].resize(numoutchans);
			PCM_source* src = PCM_Source_CreateFromFile(filestoprocess[i].toRawUTF8());
			if (src != nullptr)
			{
				int64_t lenframes = src->GetLength()*src->GetSampleRate();
				for (int j = 0; j < numoutchans; ++j)
					buffers[i][j].resize(lenframes);
				readbuf.resize(lenframes*numoutchans);
				PCM_source_transfer_t transfer = { 0 };
				transfer.length = lenframes;
				transfer.nch = numoutchans;
				transfer.samplerate = outsr;
				transfer.samples = readbuf.data();
				src->GetSamples(&transfer);
				delete src;
				for (int j = 0; j < numoutchans; ++j)
				{
					for (int k = 0; k < lenframes; ++k)
					{
						buffers[i][j][k] = readbuf[k*numoutchans + j];
					}
				}
			}
			//else
			//	showConsoleMsg("Could not open " + filestoprocess[i] + " for reading");

		}
		bool usebadconcurrency = false;
		if (usebadconcurrency == true)
		{
			std::vector<std::shared_ptr<PluginChain>> chains;
			for (int i = 0; i < numthreads; ++i)
				chains.push_back(PluginChain::createFromFile(chainfn));
			std::vector<std::shared_ptr<std::thread>> threads(numthreads);
			for (int i = 0; i < numthreads; ++i)
			{
				auto thread_func = [i, &chains, &buffers, numthreads, outsr, comp]()
				{
					for (int j = i; j < buffers.size(); j += numthreads)
					{
						if (j < buffers.size())
						{
							chains[i]->render(buffers[j], outsr, 512, nullptr, &comp->m_progresses[j]);
							MessageManager::callAsync([i, j]()
							{
								//showConsoleMsg("Thread " + String(i) + " finished " + String(j));
							});
						}
					}
				};
				threads[i] = std::make_shared<std::thread>(thread_func);
			}
			for (auto& e : threads)
				e->join();
		}
		else
		{
#ifdef WIN32
            PluginChainPool chainpool(chainfn, 8);
			Concurrency::parallel_for(0, (int)buffers.size(), [&chainpool,&buffers, comp, outsr](int i)
			{
				auto chain = chainpool.obtain();
				if (chain != nullptr)
				{
					chain->render(buffers[i], outsr, 512, nullptr, &comp->m_progresses[i]);
					chainpool.release(chain);
				}
            });
#endif
		}
		MessageManager::callAsync([comp]() 
		{
			//comp->removeFromDesktop();
			delete comp;
			//MessageManager::deleteInstance();
		});
	};
	std::thread renderth(rendertask);
	renderth.detach();
}

void testPluginChainFileRender(bool usemultithreading)
{
	if (usemultithreading == false)
	{
		String r = renderFileWithChain("C:/ProgrammingProjects/gitrepos/ReaSampleLib2/reafir_with_huge_latency.pluginchain",
			"C:/MusicAudio/sourcesamples/count.wav",
			"C:/ProgrammingProjects/gitrepos/ReaSampleLib2/chainrendertest1.wav");
		if (r.isEmpty() == false)
		{
			showConsoleMsg(r);
		}
	}
	else
	{
		renderFolderWithChainMultithreaded("C:/ProgrammingProjects/gitrepos/ReaSampleLib2/reafir_with_huge_latency.pluginchain",
			"C:/MusicAudio/Samples from net/Berklee44v2",
			"C:/MusicAudio/Samples from net/Berklee44v2/mtrendertest",4);
	}
}
