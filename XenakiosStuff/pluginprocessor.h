#pragma once

#include "JuceHeader.h"
#include <vector>
#include <memory>
#include <functional>
#include "jcomponents.h"
#include <atomic>

class MediaItem;
class PluginChainEditor;

class PluginChain
{
public:
	struct plugin_entry
	{
		plugin_entry() {}
		plugin_entry(std::shared_ptr<AudioPluginInstance> p) :
			m_plug(p) {}
		std::shared_ptr<AudioPluginInstance> m_plug;
		Image m_thumb;
	};
	PluginChain();
	~PluginChain();
	bool addPlugin(PluginDescription& plugdesc);
	void removeAllPlugins();
	void removePlugin(int index);
	int numPlugins() const { return m_plugins.size(); }
	AudioPluginInstance* getPlugin(int index);
	void render(MediaItem* item, double sr, String outfn);
	void render(std::vector<std::vector<double>>& buf, double sr, int blocksize = 512, bool* cancel_flag = nullptr, double* progress_amount = nullptr);
	PluginChain* duplicate();
	void setThumbImage(int index, Image img);
	ValueTree getState();
	void setState(ValueTree state);
	static void shutDown();
	static std::shared_ptr<PluginChain> createFromFile(String fn);
private:
	std::vector<plugin_entry> m_plugins;
	friend class PluginChainEditor;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginChain)
};

class PluginChainEditor : public Component
{
public:
	PluginChainEditor(PluginChain* chain);
	void paint(Graphics& g) override;
	void mouseDown(const MouseEvent& ev) override;
	void mouseDrag(const MouseEvent& ev) override;
	void mouseUp(const MouseEvent& ev) override;
	std::function<void(int)> PluginSelectedCallback;
	std::function<void(int)> PluginAboutToBeDeletedCallback;
private:
	PluginChain* m_chain = nullptr;
	int plugIndexFromCoord(int x, int y, int& subelem);
	int m_plug_to_drag = -1;
    int m_plugin_box_width = 120;
};

class PluginProcessingContent : public Component, 
	public Button::Listener
{
public:
	PluginProcessingContent();
	~PluginProcessingContent();
	void resized() override;
	void buttonClicked(Button* b) override;
private:
	TextEditor m_outfile_edit;
	TextButton m_render_but;
	TextButton m_plugin_choose_but;
	TextButton m_plugin_manager_but;
	TextButton m_file_but;
	std::shared_ptr<PluginChain> m_chain;
	std::shared_ptr<PluginChainEditor> m_chain_editor;
	std::unique_ptr<GenericAudioProcessorEditor> m_gen_ed;
	KnownPluginList m_pluginlist;
	int m_chosen_plugin_type = -1;
	int m_currently_visible_editor = -1;
	void initPlugList();
	void pluginSelected(int index);
	void renderSelectedItems();
	void saveChainToFile();
	void loadChainFromFile();
};

class PluginProcessingWindow : public MyWindow
{
public:
	PluginProcessingWindow(String title, int w, int h, bool resizable, Colour bgcolor);
private:
	PluginProcessingContent* m_content=nullptr;
};

void testPluginChain();

void testPluginChainFileRender(bool usemultithreading);



