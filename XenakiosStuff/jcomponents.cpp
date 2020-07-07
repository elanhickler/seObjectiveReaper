
#include "jcomponents.h"
#include "../reaper plugin/reaper_plugin_functions.h"

std::set<MyWindow*> g_windows;
bool g_final_shutdown_in_progress = false;
bool g_juce_inited = false;
std::unique_ptr<PropertiesFile> g_properties_file;

MyWindow::MyWindow(String title, int w, int h, bool allowResize, Colour bgcolor) : 
	ResizableWindow(title, bgcolor, true)
{
	m_content = std::make_shared<MyComponent>();
	setContentNonOwned(m_content.get(), true);
	setTopLeftPosition(10, 60);
	setSize(w, h);
	setResizable(allowResize, false);
	setOpaque(true);
	g_windows.insert(this);
	
#ifndef WIN32
    // Dirty hack for OSX to keep the window inside Reaper
    // To do this properly requires some Cocoa/Obj-C code in a separate file...Sigh.
	//setUsingNativeTitleBar(true);
	//setAlwaysOnTop(true);
#endif
}

MyWindow::~MyWindow()
{
	//if (SaveCallback)
	//	SaveCallback(this);
	if (g_final_shutdown_in_progress == false)
	{
		if (g_windows.count(this) > 0)
			g_windows.erase(this);
	}
	else
	{
		//HWND h = (HWND)getPeer()->getNativeHandle();
		//SetParent(h, 0);
	}
}

void MyWindow::addLabel(String id, int x, int y, int w, int h, String initText)
{
	if (m_content != nullptr)
		m_content->addLabel(id, x, y, w, h, initText);
}

void MyWindow::addCodeEdit(String id, int x, int y, int w, int h, String inittext, Colour bgcolor, Colour txtcolor)
{
	if (m_content != nullptr)
		m_content->addCodeEdit(id, x, y, w, h, inittext, bgcolor, txtcolor);
}

void MyWindow::addMultiLineTextEdit(String id, int x, int y, int w, int h, String inittext, Colour bgcolor, Colour txtcolor)
{
	if (m_content != nullptr)
		m_content->addMultiLineTextEdit(id, x, y, w, h, inittext, bgcolor, txtcolor);
}

void MyWindow::addSingleLineTextEdit(String id, int x, int y, int w, int h, String inittext, Colour bgcolor, Colour txtcolor)
{
	if (m_content != nullptr)
		m_content->addSingleLineTextEdit(id, x, y, w, h, inittext, bgcolor, txtcolor);
}

void MyWindow::addButton(String id, int x, int y, int w, int h, String text, std::function<void(String, var)> callback)
{
	if (m_content != nullptr)
		m_content->addButton(id, x, y, w, h, text, callback);
}

void MyWindow::addImageButton(String id, int x, int y, int w, int h, Image img, std::function<void(String, var)> callback)
{
	if (m_content != nullptr)
		m_content->addImageButton(id, x, y, w, h, img, callback);
}

void MyWindow::addImageButton(String id, int x, int y, Image img, std::function<void(String, var)> callback)
{
	if (m_content != nullptr)
		m_content->addImageButton(id, x, y, 0, 0, img, callback);
}

void MyWindow::addCheckBox(String id, int x, int y, int w, int h, String text, bool initstate, 
	std::function<void(String, var)> callback)
{
	if (m_content != nullptr)
		m_content->addCheckBox(id, x, y, w, h, text, initstate, callback);
}

void MyWindow::addComboBox(String id, int x, int y, int w, int h, StringArray options, int initstate, 
	std::function<void(String, var)> callback)
{
	if (m_content != nullptr)
		m_content->addComboBox(id, x, y, w, h, options, initstate, callback);
}

void MyWindow::addPresetComponent(String id, int x, int y, int w, int h, String initialtext)
{
	if (m_content != nullptr)
		m_content->addPresetComponent(id, x, y, w, h, initialtext);
}

void MyWindow::addRadioButtons(String id, int x, int y, int w, int h, String title, StringArray options, int initstate)
{
	if (m_content != nullptr)
		m_content->addRadioButtons(id, x, y, w, h, title, options, initstate);
}

void MyWindow::addRadioButtons(String id, int x, int y, int w, int h, std::initializer_list<RadioButtonsComponent::option_entry> options, String title, int initstate)
{
	if (m_content != nullptr)
		m_content->addRadioButtons(id, x, y, w, h, options, title, initstate);
}

void MyWindow::setTipTextForComponent(String id, String tiptext)
{
	if (m_content != nullptr)
		m_content->setTipTextForComponent(id, tiptext);
}

var MyWindow::getComponentValue(String id, String propname)
{
	if (m_content != nullptr)
		return m_content->getComponentValue(id, propname);
	return var();
}

void MyWindow::setComponentValue(String id, var v, String propname)
{
	if (m_content != nullptr)
		m_content->setComponentValue(id, v, propname);
}

void MyWindow::userTriedToCloseWindow()
{
	if (SaveCallback)
		SaveCallback(this);
	if (CloseCallback)
		CloseCallback(this);
	else setVisible(false);
	if (m_destroy_on_close == true)
	{
		delete this; // argh, hopefully this won't lead to some mess at some point...
	}
#ifdef WIN32
	// Attempt to work around the sporadic problem where Reaper's window loses focus
	// or it even disappears/minimizes when the Juce window is closed
	BringWindowToTop(GetMainHwnd());
#endif
}

void MyWindow::visibilityChanged()
{
	ResizableWindow::visibilityChanged();
	if (isVisible() == false && SaveCallback)
		SaveCallback(this);
}

bool MyWindow::keyPressed(const KeyPress & kp)
{
	if (kp == KeyPress::escapeKey)
	{
		userTriedToCloseWindow();
		return true;
	}
	return false;
}

ValueTree MyWindow::saveState()
{
	String temp = getName().replaceCharacter(' ', '_');
	ValueTree result(temp);
	result.setProperty("wx", getX(), nullptr);
	result.setProperty("wy", getY(), nullptr);
	if (m_content != nullptr)
		result.addChild(m_content->saveState(),-1,nullptr);
	return result;
}

String MyWindow::loadState(ValueTree vt)
{
	if (vt.isValid() == false)
		return "ValueTree is not valid";
	Identifier temp = getName().replaceCharacter(' ', '_');
	if (vt.getType()!=temp)
		return "State data is not for window " + temp;
	int x = vt.getProperty("wx", 10);
	int y = vt.getProperty("wy", 60);
	setTopLeftPosition(x, y);
	if (m_content != nullptr)
		return m_content->loadState(vt);
	return String();
}

void MyWindow::deleteAllWindowInstances()
{
	//Logger::writeToLog("At shutdown there are " + String(g_windows.size() + " window instances to delete"));
	g_final_shutdown_in_progress = true;
	for (auto& e : g_windows)
	{
		e->removeFromDesktop();
		e->setVisible(false);
		delete e;
	}
	g_windows.clear();
}

void MyWindow::doGUIShutdown()
{
	if (g_juce_inited == true)
	{
		g_properties_file->saveIfNeeded();
		g_properties_file = nullptr;
		shutdownJuce_GUI();
		g_juce_inited = false;
	}
}

MyComponent::MyComponent()
{
	setSize(300, 100);
	startTimer(1000, 100);
}

MyComponent::~MyComponent()
{
	stopTimer(1000);
}

void MyComponent::addLabel(String id, int x, int y, int w, int h, String initText)
{
	auto label = std::make_shared<Label>(id, initText);
	label->setBounds(x, y, w, h);
	label->setComponentID(id);
	addAndMakeVisible(label.get());
	ComponentEntry entry;
	entry.m_type = CT_Label;
	entry.m_component = label;
	m_components.push_back(entry);
	if (id == "infolabel")
	{
		m_infolabel_entry_index = m_components.size() - 1;
		m_info_label_normal_colour = label->findColour(Label::textColourId);
	}
}

void MyComponent::addCodeEdit(String id, int x, int y, int w, int h ,String inittext,
	Colour bgcolor, Colour txtcolor)
{
	auto ed = std::make_shared<RSLCodeEditor>();
	ed->setBounds(x, y, w, h);
	ed->setText(inittext);
	ed->setComponentID(id);
	ed->setColour(TextEditor::backgroundColourId, bgcolor);
	ed->setColour(TextEditor::textColourId, txtcolor);
	addAndMakeVisible(ed.get());
	ComponentEntry entry;
	entry.m_type = CT_CodeEdit;
	entry.m_component = ed;
	m_components.push_back(entry);
}

void MyComponent::addMultiLineTextEdit(String id, int x, int y, int w, int h, String inittext, Colour bgcolor, Colour txtcolor)
{
	auto ed = std::make_shared<RSLMultiLineEditor>();
	ed->setEscapeAndReturnKeysConsumed(true);
	ed->setTabKeyUsedAsCharacter(true);
	ed->setReturnKeyStartsNewLine(true);
	ed->setBounds(x, y, w, h);
	ed->setText(inittext);
	ed->setComponentID(id);
	ed->setColour(TextEditor::backgroundColourId, bgcolor);
	ed->setColour(TextEditor::textColourId, txtcolor);
	addAndMakeVisible(ed.get());
	ComponentEntry entry;
	entry.m_type = CT_MultiLineTextEdit;
	entry.m_component = ed;
	m_components.push_back(entry);
}

void MyComponent::addSingleLineTextEdit(String id, int x, int y, int w, int h, String inittext, Colour bgcolor, Colour txtcolor)
{
	auto ed = std::make_shared<TextEditor>();
	ed->setBounds(x, y, w, h);
	ed->setText(inittext);
	ed->setComponentID(id);
	ed->setColour(TextEditor::backgroundColourId, bgcolor);
	ed->setColour(TextEditor::textColourId, txtcolor);
	addAndMakeVisible(ed.get());
	ComponentEntry entry;
	entry.m_type = CT_LineEdit;
	entry.m_component = ed;
	m_components.push_back(entry);
}

void MyComponent::addButton(String id, int x, int y, int w, int h, String text, std::function<void(String,var)> callback)
{
	auto but = std::make_shared<TextButton>(id);
	but->setBounds(x, y, w, h);
	but->setButtonText(text);
	but->setComponentID(id);
	but->addListener(this);
	addAndMakeVisible(but.get());
	ComponentEntry entry;
	entry.m_type = CT_Button;
	entry.m_component = but;
	entry.m_var_callback = callback;
	m_components.push_back(entry);
	
}

void MyComponent::addImageButton(String id, int x, int y, int w, int h, Image img, std::function<void(String, var)> callback)
{
	auto but = std::make_shared<ImageButton>(id);
	if (w==0 && h == 0)
		but->setBounds(x, y, 1, 1);
	else 
		but->setBounds(x, y, w, h);
	but->setImages(true, true, true, img,
		0.75f, Colours::transparentWhite, 
		img, 1.0f, Colours::transparentWhite, img, 0.75f, Colours::transparentWhite);
	but->setComponentID(id);
	but->addListener(this);
	addAndMakeVisible(but.get());
	ComponentEntry entry;
	entry.m_type = CT_Button;
	entry.m_component = but;
	entry.m_var_callback = callback;
	m_components.push_back(entry);
}

void MyComponent::addCheckBox(String id, int x, int y, int w, int h, String text, bool initstate,
	std::function<void(String, var)> callback)
{
	auto but = std::make_shared<ToggleButton>(id);
	but->setBounds(x, y, w, h);
	but->setButtonText(text);
	but->setComponentID(id);
	but->addListener(this);
	addAndMakeVisible(but.get());
	ComponentEntry entry;
	entry.m_type = CT_CheckBox;
	entry.m_component = but;
	entry.m_var_callback = callback;
	m_components.push_back(entry);
}

void MyComponent::addComboBox(String id, int x, int y, int w, int h, StringArray options, int initstate, 
	std::function<void(String, var)> callback)
{
	auto combo = std::make_shared<ComboBox>();
	combo->setBounds(x, y, w, h);
	combo->setComponentID(id);
	combo->addListener(this);
	combo->addItemList(options, 1);
	combo->setSelectedItemIndex(initstate);
	addAndMakeVisible(combo.get());
	ComponentEntry entry;
	entry.m_type = CT_ComboBox;
	entry.m_component = combo;
	entry.m_var_callback = callback;
	m_components.push_back(entry);
}

String MyComponent::getParentNameWithUnderScores()
{
	if (getParentComponent() == nullptr)
		return String();
	return getParentComponent()->getName().replaceCharacter(' ', '_');
}

void MyComponent::addRadioButtons(String id, int x, int y, int w, int h, String title,StringArray options, int initstate)
{
	auto radios = std::make_shared<RadioButtonsComponent>(id,title,options,initstate);
	radios->setBounds(x, y, w, h);
	radios->setComponentID(id);
	addAndMakeVisible(radios.get());
	ComponentEntry entry;
	entry.m_type = CT_RadioButtons;
	entry.m_component = radios;
	m_components.push_back(entry);
}

void MyComponent::addRadioButtons(String id, int x, int y, int w, int h, 
	std::initializer_list<RadioButtonsComponent::option_entry> options, String title, int initstate)
{
	auto radios = std::make_shared<RadioButtonsComponent>(id, options, title, initstate);
	radios->setBounds(x, y, w, h);
	radios->setComponentID(id);
	addAndMakeVisible(radios.get());
	ComponentEntry entry;
	entry.m_type = CT_RadioButtons;
	entry.m_component = radios;
	m_components.push_back(entry);
}

void MyComponent::addPresetComponent(String id, int x, int y, int w, int h, String initialtext)
{
	auto combo = std::make_shared<PresetComponent>(id, initialtext);
	combo->setBounds(x, y, w, h);
	combo->setComponentID(id);
	addAndMakeVisible(combo.get());
	ComponentEntry entry;
	entry.m_type = CT_PresetComponent;
	entry.m_component = combo;
	m_components.push_back(entry);
}

var MyComponent::getComponentValue(String id, String propname)
{
	for (auto& e : m_components)
	{
		if (id == e.m_component->getComponentID())
		{
			if (e.m_type == CT_CheckBox && propname.isEmpty())
				return e.getAs<ToggleButton>()->getToggleState();
			if (e.m_type == CT_ComboBox && propname.isEmpty())
				return e.getAs<ComboBox>()->getSelectedId();
			if (e.m_type == CT_LineEdit && propname.isEmpty())
				return e.getAs<TextEditor>()->getText();
			if (e.m_type == CT_CodeEdit && propname.isEmpty())
				return e.getAs<RSLCodeEditor>()->getText();
			if (e.m_type == CT_Slider && propname.isEmpty())
				return e.getAs<Slider>()->getValue();
			if (e.m_type == CT_Slider && propname == "low")
				return e.getAs<Slider>()->getMinimum();
			if (e.m_type == CT_Slider && propname == "high")
				return e.getAs<Slider>()->getMaximum();
			if (e.m_type == CT_RadioButtons && propname.isEmpty())
				return e.getAs<RadioButtonsComponent>()->getSelectedButton();
			if (e.m_type == CT_PresetComponent && propname.isEmpty())
				return e.getAs<PresetComponent>()->getValue();
		}

	}
	return var();
}

void MyComponent::setComponentValue(String id, var v, String propname)
{
	for (auto& e : m_components)
	{
		if (id == e.m_component->getComponentID())
		{
			if (e.m_type == CT_CodeEdit && propname.isEmpty())
				e.getAs<RSLCodeEditor>()->setText(v);
			if (e.m_type == CT_LineEdit && propname.isEmpty())
				e.getAs<TextEditor>()->setText(v);
			if (e.m_type == CT_Label && propname.isEmpty())
				e.getAs<Label>()->setText(v,dontSendNotification);
			// These have to be rewritten for use with CodeEditorComponent, or removed if it doesn't support the operations...
			/*
			if (e.m_type == CT_TextEdit && propname == "insert")
				e.getAs<MyTextEditor>()->insertTextAtCaret(v);
			if (e.m_type == CT_TextEdit && propname == "appendtext")
			{
				auto ed = e.getAs<MyTextEditor>();
				ed->setText(ed->getText() + v.toString());
			}
			*/
		}
	}
}

void MyComponent::setTipTextForComponent(String id, String tiptext)
{
	Component* c = getComponentFromID(id);
	if (c != nullptr)
		c->getProperties().set("tiptext", tiptext);
}

void MyComponent::setInfoLabelText(String text, bool redtext)
{
	if (m_infolabel_entry_index >= 0)
	{
		Label* lab = m_components[m_infolabel_entry_index].getAs<Label>();
		lab->setText(text, dontSendNotification);
		if (redtext == true)
		{
			lab->setColour(Label::textColourId, Colours::red);
			m_info_text_is_error = true;
		}
	}
}

void MyComponent::buttonClicked(Button * button)
{
	for (auto& e : m_components)
	{
		if (e.m_component.get() == button)
		{
			String id = button->getComponentID();
			if (e.m_type == CT_Button)
			{
				e.m_var_callback(id,true);
				return;
			}
			if (e.m_type == CT_CheckBox)
			{
				e.m_var_callback(id, button->getToggleState());
				return;
			}
		}
	}
}

void MyComponent::comboBoxChanged(ComboBox * combo)
{
	for (auto& e : m_components)
	{
		if (e.m_component.get() == combo)
		{
			String id = combo->getComponentID();
			if (e.m_type == CT_ComboBox)
			{
				e.m_var_callback(id, combo->getSelectedItemIndex());
				return;
			}
		}
	}
}

MyComponent::ComponentEntry * MyComponent::getComponentEntryForID(String id)
{
	for (auto& e : m_components)
		if (e.m_component->getComponentID() == id)
			return &e;
	return nullptr;
}

void MyComponent::resized()
{
	for (auto& e : m_components)
	{
		if (e.m_resize_callback)
		{
			juce::Rectangle<int> r = e.m_resize_callback(this);
			e.m_component->setBounds(r);
		}
	}
}

ValueTree MyComponent::saveState()
{
	ValueTree result("genericcontent");
	for (auto& e : m_components)
	{
		Identifier key = e.m_component->getComponentID();
		ValueTree state(key);
		if (e.m_type == CT_CodeEdit)
			state.setProperty(key, e.getAs<RSLCodeEditor>()->getText(),nullptr);
		if (e.m_type == CT_LineEdit)
			state.setProperty(key, e.getAs<TextEditor>()->getText(), nullptr);
		if (e.m_type == CT_RadioButtons)
			state.setProperty(key, e.getAs<RadioButtonsComponent>()->getSelectedButton(), nullptr);
		if (e.m_type == CT_Slider)
			state.setProperty(key, e.getAs<Slider>()->getValue(), nullptr);
		if (e.m_type == CT_PresetComponent)
			state.addChild(e.getAs<PresetComponent>()->getState(), -1,nullptr);
		if (state.getNumProperties()>0 || state.getNumChildren()>0)
			result.addChild(state, -1, nullptr);
	}
	return result;
}

String MyComponent::loadState(ValueTree source)
{
	ValueTree temp = source.getChildWithName("genericcontent");
	if (temp.isValid() == false)
		return "Generic child component state was not found";
	for (auto& e : m_components)
	{
		Identifier id = e.m_component->getComponentID();
		ValueTree state = temp.getChildWithName(id);
		if (state.isValid())
		{
			juce::var prop = state.getProperty(id);
			if (prop.isVoid() == false)
			{
				if (e.m_type == CT_CodeEdit)
					e.getAs<RSLCodeEditor>()->setText(prop);
				if (e.m_type == CT_LineEdit)
					e.getAs<TextEditor>()->setText(prop);
				if (e.m_type == CT_RadioButtons)
					e.getAs<RadioButtonsComponent>()->setSelectedButton(prop);
				if (e.m_type == CT_Slider)
					e.getAs<Slider>()->setValue(prop, dontSendNotification);
				if (e.m_type == CT_CheckBox)
					e.getAs<ToggleButton>()->setToggleState(prop, dontSendNotification);
				if (e.m_type == CT_ComboBox)
					e.getAs<ComboBox>()->setSelectedItemIndex(prop, dontSendNotification);
			}
			if (e.m_type == CT_PresetComponent)
			{
				e.getAs<PresetComponent>()->setState(state.getChildWithName("presetdata"));
			}
		}
		
	}
	return String();
}

void MyComponent::updateInfoLabel()
{
	if (m_infolabel_entry_index < 0)
		return;
	auto pt = getMouseXYRelative();
	Component* foo = getComponentAt(pt);
	if (foo != nullptr)
	{
		String tiptext;
		Label* lab = m_components[m_infolabel_entry_index].getAs<Label>();
		if (foo != this)
		{
			String compid = foo->getComponentID();
			if (m_last_detected_tip_component != compid)
			{
				m_last_detected_tip_component = compid;
				Component* tipsourcecomponent = foo;
				if (foo->getParentComponent() != nullptr && foo->getParentComponent() != this)
					tipsourcecomponent = foo->getParentComponent();
				tiptext = tipsourcecomponent->getProperties()["tiptext"];
				if (m_info_text_is_error == true)
				{
					m_info_text_is_error = false;
					lab->setColour(Label::textColourId, m_info_label_normal_colour);
				}
				if (tiptext.isEmpty() == false)
					lab->setText(tiptext, dontSendNotification);
				else lab->setText("", dontSendNotification);
			}
		}
		else
		{
			lab->setText("", dontSendNotification);
			m_last_detected_tip_component = "";
		}
		
	}
}

void MyComponent::timerCallback(int id)
{
	if (id == 1000)
	{
		updateInfoLabel();
	}
}

void MyWindow::initGuiIfNeeded()
{
	if (g_juce_inited == false)
	{
		initialiseJuce_GUI();
		PropertiesFile::Options poptions;
		poptions.applicationName = "reasamplelib";
		poptions.folderName = "Soundemote";
		poptions.commonToAllUsers = false;
		poptions.doNotSave = false;
		poptions.storageFormat = PropertiesFile::storeAsXML;
		poptions.millisecondsBeforeSaving = 1000;
		poptions.ignoreCaseOfKeyNames = false;
		poptions.processLock = nullptr;
		poptions.filenameSuffix = ".xml";
		poptions.osxLibrarySubFolder = "Application Support";

		g_properties_file = std::make_unique<PropertiesFile>(poptions);
		g_juce_inited = true;
	}
}

void saveWindowStateIntoGlobalSettings(String windowname,ValueTree vt)
{
	if (windowname.isEmpty() == true)
		return;
	String sectname = windowname.replaceCharacter(' ', '_');
	auto xml = vt.createXml();
	if (xml != nullptr)
	{
		g_properties_file->setValue(sectname, xml.get());
	}
}

ValueTree loadWindowStateFromGlobalSettings(String windowname)
{
	String sectname = windowname.replaceCharacter(' ', '_');

	initialiseJuce_GUI();
	PropertiesFile::Options poptions;
	poptions.applicationName = "reasamplelib";
	poptions.folderName = "Soundemote";
	poptions.commonToAllUsers = false;
	poptions.doNotSave = false;
	poptions.storageFormat = PropertiesFile::storeAsXML;
	poptions.millisecondsBeforeSaving = 1000;
	poptions.ignoreCaseOfKeyNames = false;
	poptions.processLock = nullptr;
	poptions.filenameSuffix = ".xml";
	poptions.osxLibrarySubFolder = "Application Support";

	auto properties_file = std::make_unique<PropertiesFile>(poptions);

	auto xml = properties_file->getXmlValue(sectname);

	if (xml != nullptr)
	{
		ValueTree r = ValueTree::fromXml(*xml);
		return r;
	}

  return ValueTree();
}

void saveWindowStateIntoReaperProject(NamedValueSet nvs)
{
	String sectname = nvs["windowname"];
	if (sectname.isEmpty() == true)
		return;
	for (int i = 0; i < nvs.size(); ++i)
	{
		auto key = nvs.getName(i);
		if (key == Identifier("windowname"))
			continue;
		String temp = nvs.getValueAt(i);
		SetProjExtState(nullptr, sectname.toRawUTF8(), key.getCharPointer(), temp.toRawUTF8());
	}
}

Image loadImage(String imgname, ReaperFileLocation location)
{
	if (location == ReaperFileLocation::ReaperResourceDir)
		return ImageFileFormat::loadFrom(String(CharPointer_UTF8(GetResourcePath())) + imgname);
	if (location == ReaperFileLocation::ReaperPluginDir)
		return ImageFileFormat::loadFrom(String(CharPointer_ASCII(GetExePath())) +"/Plugins/" + imgname);
	if (location == ReaperFileLocation::ReaperThemeDir)
		return ImageFileFormat::loadFrom(String(CharPointer_UTF8(GetResourcePath())) + "/ColorThemes/" + imgname);
	return ImageFileFormat::loadFrom(File(imgname));
}

RadioButtonsComponent::RadioButtonsComponent(String id, String title, StringArray texts, int initsel)
{
	m_label.setText(title, dontSendNotification);
	m_label.setComponentID(id);
	addAndMakeVisible(m_label);
	setComponentID(id);
	for (int i = 0; i<texts.size(); ++i)
	{
		auto but = std::make_shared<ToggleButton>();
		but->setButtonText(texts[i]);
		if (i == initsel)
			but->setToggleState(true, dontSendNotification);
		but->setRadioGroupId(1);
		but->addListener(this);
		addAndMakeVisible(but.get());
		but->setComponentID(id);
		m_buttons.push_back(but);
	}
	m_current_but = initsel;
}

RadioButtonsComponent::RadioButtonsComponent(String id, 
	std::initializer_list<option_entry> entries, String title,int initsel)
{
	if (title.isEmpty() == false)
	{
		m_label.setText(title, dontSendNotification);
		m_label.setComponentID(id);
		addAndMakeVisible(m_label);
	}
	setComponentID(id);
	for (int i = 0; i<entries.size(); ++i)
	{
		auto but = std::make_shared<ToggleButton>();
		const option_entry* entry = entries.begin() + i;
		but->setButtonText(entry->m_text);
		if (i == initsel)
			but->setToggleState(true, dontSendNotification);
		but->setRadioGroupId(1);
		but->addListener(this);
		addAndMakeVisible(but.get());
		if (entry->m_w > 0 && entry->m_h > 0)
			but->setBounds(entry->m_x, entry->m_y, entry->m_w, entry->m_h);
		int butid = m_buttons.size();
		if (entry->m_id >= 0)
			butid = entry->m_id;
		but->getProperties().set("__radioid", butid);
		but->setComponentID(id);
		m_buttons.push_back(but);
	}
	m_current_but = initsel;
}

int RadioButtonsComponent::getSelectedButton() const
{
	return m_current_but;
}

void RadioButtonsComponent::setSelectedButton(int b)
{
	for (int i = 0; i < m_buttons.size(); ++i)
	{
		if ((int)m_buttons[i]->getProperties()["__radioid"] == b)
		{
			m_current_but = b;
			m_buttons[i]->setToggleState(true, dontSendNotification);
			return;
		}
	}
}

void RadioButtonsComponent::buttonClicked(Button * but)
{
	for (int i = 0; i < (int)m_buttons.size(); ++i)
	{
		if (but == m_buttons[i].get())
		{
			m_current_but = but->getProperties()["__radioid"];
			return;
		}
	}
}

void RadioButtonsComponent::resized()
{
	if (m_label.getText().isEmpty() == true)
		return;
	m_label.setBounds(0, 0, getWidth(), 20);
	double buth = (double)(getHeight() - 20) / m_buttons.size();
	for (size_t i = 0; i < m_buttons.size(); ++i)
	{
		m_buttons[i]->setBounds(15, m_label.getBottom() + 1 + i*buth, getWidth() - 17, buth);
	}
}

PresetComponent::PresetComponent(String id, String initialtext) : m_datamodel("presetdata")
{
	m_combo = std::make_unique<ComboBox>();
	addAndMakeVisible(m_combo.get());
	PresetChanged = [](ValueTree) {};
	GetPresetData = [this]() 
	{ 
		double temp = m_combo->getText().getDoubleValue();
		if (fabs(temp) > 0.0001)
		{
			ValueTree vt("itemstate");
			vt.setProperty("name", String(temp, 3), nullptr);
			return vt;
		}
		return ValueTree(); 
	};
	setComponentID(id);
	
	if (initialtext.isEmpty() == false)
	{
		ValueTree vt("itemstate");
		vt.setProperty("name", initialtext, nullptr);
		m_datamodel.addChild(vt,-1,nullptr);
	}
	updateComboFromModel();
	m_combo->setEditableText(true);
	m_combo->setSelectedItemIndex(0, dontSendNotification);
    m_combo->addListener(this);

	addAndMakeVisible(&m_add_but);
	m_add_but.setComponentID(id);
	m_add_but.getProperties().set("tiptext", "Add current value into dropdown list");
	m_add_but.setButtonText("+");
	m_add_but.addListener(this);
	addAndMakeVisible(&m_remove_but);
	m_remove_but.getProperties().set("tiptext", "Remove current value from dropdown list if it is already there");
	m_remove_but.setComponentID(id);
	m_remove_but.setButtonText("-");
	m_remove_but.addListener(this);
}

void PresetComponent::resized()
{
	int w = getWidth();
	int h = getHeight();
	m_combo->setBounds(0, 0, w - 60, h);
	m_add_but.setBounds(m_combo->getRight()+5, 0, 25, h);
	m_remove_but.setBounds(m_add_but.getRight()+5, 0, 25, h);
}

void PresetComponent::buttonClicked(Button * but)
{
	if (but == &m_add_but)
	{
		ValueTree state = GetPresetData();
		if (state.isValid())
		{
			if (state.hasProperty("name") == false)
				state.setProperty("name", m_combo->getText(), nullptr);
			int childcount = m_datamodel.getNumChildren();
			int exists_index = -1;
			String name = state.getProperty("name");
			for (int i = 0; i < childcount; ++i)
			{
				ValueTree vt = m_datamodel.getChild(i);
				if (vt.hasProperty("name") && vt.getProperty("name") == name)
				{
					exists_index = i;
					break;
				}
			}
			if (exists_index == -1)
				m_datamodel.addChild(state, -1, nullptr);
			else
				m_datamodel.getChild(exists_index).copyPropertiesFrom(state, nullptr);
			updateComboFromModel();
		}
	}
	if (but == &m_remove_but)
	{
		removeSelectedItem();
	}
}

var PresetComponent::getValue()
{ 
	return m_combo->getText(); 
}

ValueTree PresetComponent::getState()
{
	m_datamodel.setProperty("seltext", m_combo->getText(), nullptr);
	return m_datamodel;
}

void PresetComponent::setState(ValueTree vt)
{
	if (vt.isValid() == false)
		return;
	m_datamodel = vt;
	updateComboFromModel();
	m_combo->setText(vt.getProperty("seltext"), dontSendNotification);
}

void PresetComponent::removeSelectedItem()
{
	int index = m_combo->getSelectedItemIndex();
	if (index >= 0)
	{
		m_datamodel.removeChild(index, nullptr);
		updateComboFromModel();
		return;
	}
}

void PresetComponent::updateCurrentPreset()
{
	int index = m_combo->getSelectedItemIndex();
	if (index >= 0)
	{
		ValueTree state = GetPresetData();
		if (state.isValid())
			m_datamodel.getChild(index).copyPropertiesFrom(state, nullptr);
	}
}

void PresetComponent::comboBoxChanged(ComboBox * combo)
{
	if (combo == m_combo.get() && PresetChanged)
	{
		if (combo->getSelectedItemIndex()>=0)
			PresetChanged(m_datamodel.getChild(combo->getSelectedItemIndex()));
	}
}

void PresetComponent::updateComboFromModel()
{
	if (m_datamodel.isValid() == false)
		return;
	sortModel();
	m_combo->clear(dontSendNotification);
	int numchildren = m_datamodel.getNumChildren();
	for (int i = 0; i < numchildren; ++i)
	{
		ValueTree child = m_datamodel.getChild(i);
		m_combo->addItem(child.getProperty("name"), i + 1);
	}
}

struct PresetComboModelComparator
{
	enum Order
	{
		ByText,
		ByDataValue
	};
	PresetComboModelComparator(Order ord=ByText) : m_order(ord) {}
	int compareElements(const ValueTree& lhs, const ValueTree& rhs)
	{
		if (m_order == ByText)
		{
			return lhs.getProperty("name").toString().compare(rhs.getProperty("name").toString());
		}
		return 0;
	}
	Order m_order = ByText;
};

void PresetComponent::sortModel()
{
	PresetComboModelComparator comp;
	m_datamodel.sort(comp, nullptr, true);
}

void RSLCodeEditor::textChanged()
{
	if (TextChangedCallback)
		TextChangedCallback(code.getAllContent());
}
