#pragma once

#include "JuceHeader.h"

using namespace juce;

#ifndef WIN32
void makeWindowFloatingPanel(Component *aComponent);
#endif

// It is probably going to be useful to force custom components to derive from this,
// for type reflection and state save/load purposes

class CustomComponentBase : public Component
{
public:
	virtual String getClassName() const = 0;
	// This should return the main value of interest of the component, like selected text or index of active 
	// item etc
	virtual var getValue() = 0; 
	// This should return the full state of the component so that it can later be restored in a setState call
	virtual ValueTree getState() = 0;
	// This should restore the full state of the component, previously got from a call to getState
	virtual void setState(ValueTree state) = 0;
};

class CallbackAction
{
public:
	using action_callback_t = std::function<void(void)>;
	CallbackAction() {}
	CallbackAction(String desc, action_callback_t cb) 
		: m_desc(desc), m_callback(cb) {}
	String m_desc;
	action_callback_t m_callback;
};

class RSLMultiLineEditor : public TextEditor
{
public:
	void setContextMenuActions(std::vector<CallbackAction> actions) 
	{ 
		m_actions = actions; 
	}
private:
	void addPopupMenuItems(PopupMenu &menuToAddTo, const MouseEvent *mouseClickEvent)  override
	{
		TextEditor::addPopupMenuItems(menuToAddTo, mouseClickEvent);

		for (int i = 0; i < m_actions.size(); ++i)
			menuToAddTo.addItem(i + 100, m_actions[i].m_desc);
	}

	void performPopupMenuAction(int menuItemID) override
	{
		if (menuItemID >= 100 && menuItemID < 100 + m_actions.size())
			m_actions[menuItemID - 100].m_callback();
		else
			TextEditor::performPopupMenuAction(menuItemID);
	}
	std::vector<CallbackAction> m_actions;
};

/**
@note - we cannot directly inherit from CodeEditorComponent because otherwise the code object isn't constructed
before the CodeEditorComponent and we get a crash.  Hence the awkward nested Component.  There's probably a tidier 
soution but this solution didn't involve changing any of the rest of the code base.
*/
class RSLCodeEditor : public Component, public CodeDocument::Listener
{
public:
	RSLCodeEditor()
	{
		code.addListener(this);
		editor.setLineNumbersShown(true);
		addAndMakeVisible(editor);
	}

	void resized() override { editor.setBounds(getLocalBounds()); }

	void setContextMenuActions(std::vector<CallbackAction> actions) { editor.setContextMenuActions(actions); }

	class CustomCodeEditor
		:
		public CodeEditorComponent
	{
	public:
		CustomCodeEditor(CodeDocument& document, CodeTokeniser* codeTokeniser) : CodeEditorComponent(document, codeTokeniser) {} 
		void setContextMenuActions(const std::vector<CallbackAction> & actions) { m_actions = actions; }

	private:
		void addPopupMenuItems(PopupMenu &menuToAddTo, const MouseEvent *mouseClickEvent)  override
		{
			CodeEditorComponent::addPopupMenuItems(menuToAddTo, mouseClickEvent);

			for (int i = 0; i < m_actions.size(); ++i)
				menuToAddTo.addItem(i + 100, m_actions[i].m_desc);
		}

		void performPopupMenuAction(int menuItemID) override
		{
			if (menuItemID >= 100 && menuItemID < 100 + m_actions.size())
				m_actions[menuItemID - 100].m_callback();
			else
				CodeEditorComponent::performPopupMenuAction(menuItemID);
		}
		std::vector<CallbackAction> m_actions;
	};

	void setText(const String& string) { code.replaceAllContent(string); }
	String getText() const { return code.getAllContent(); }
	std::function<void(String)> TextChangedCallback;

private:
	void codeDocumentTextInserted(const String&, int) override { textChanged(); }
	void codeDocumentTextDeleted(int, int) override { textChanged(); }
	void textChanged();

	CodeDocument code;
	CustomCodeEditor editor{ code, nullptr };
}; 

class PresetComponent : public Component, public Button::Listener, public ComboBox::Listener
{
public:
	PresetComponent(String id, String initialtext);
	void resized() override;
	void buttonClicked(Button* but) override;
	var getValue();
	ValueTree getState();
	void setState(ValueTree vt);
	void removeSelectedItem();
	void updateCurrentPreset();
	void comboBoxChanged(ComboBox* combo) override;
	std::function<void(ValueTree)> PresetChanged;
	std::function<ValueTree(void)> GetPresetData;
private:
	std::unique_ptr<ComboBox> m_combo;
	TextButton m_add_but;
	TextButton m_remove_but;
	ValueTree m_datamodel;
	void updateComboFromModel();
	void sortModel();
};

inline ValueTree getPresetDataFromNumberCombo(ComboBox* cb)
{
	double temp = cb->getText().getDoubleValue();
	if (fabs(temp) > 0.0001)
	{
		ValueTree vt("itemstate");
		vt.setProperty("name", String(temp, 3), nullptr);
		return vt;
	}
	return ValueTree();
}

class RadioButtonsComponent : public Component, public Button::Listener
{
public:
	struct option_entry
	{
		option_entry() {}
		option_entry(String text) : m_text(text) {}
		option_entry(int x, int y, int w, int h, String text, int id=-1) : 
			m_x(x), m_y(y), m_w(w), m_h(h), m_id(id), m_text(text) {}
		int m_x = 0;
		int m_y = 0;
		int m_w = 0;
		int m_h = 0;
		int m_id = -1;
		String m_text;
	};
	RadioButtonsComponent(String id, String title, StringArray texts, int initsel);
	RadioButtonsComponent(String id, std::initializer_list<option_entry> entries, String title, int initsel);
	int getSelectedButton() const;
	void setSelectedButton(int b);
	void buttonClicked(Button* but) override;
	void resized() override;
private:
	Label m_label;
	std::vector<std::shared_ptr<ToggleButton>> m_buttons;
	int m_current_but = -1;
};

class MyComponent : public Component, 
	public Button::Listener, public ComboBox::Listener, public MultiTimer
{
public:
	enum ComponentType
	{
		CT_Label,
		CT_MultiLineTextEdit,
		CT_CodeEdit,
		CT_LineEdit,
		CT_Button,
		CT_CheckBox,
		CT_ComboBox,
		CT_PresetComponent,
		CT_Slider,
		CT_RadioButtons,
		CT_Custom,
		CT_Unknown
	};
	struct ComponentEntry
	{
		ComponentEntry() { m_var_callback = [](String, var) {}; }
		ComponentType m_type = CT_Unknown;
		std::shared_ptr<Component> m_component;
		std::function<void(String, var)> m_var_callback;
		
		std::function<juce::Rectangle<int>(Component*)> m_resize_callback;
		template<typename T>
		inline T* getAs()
		{
            static_assert(std::is_same<T,RSLCodeEditor>::value || std::is_same<T,PresetComponent>::value ||
                          std::is_same<T,Component>::value || std::is_same<T,Slider>::value ||
                          std::is_same<T,TextButton>::value || std::is_same<T,ImageButton>::value ||
                          std::is_same<T,ToggleButton>::value || std::is_same<T,Label>::value ||
                          std::is_same<T,RadioButtonsComponent>::value || std::is_same<T,ComboBox>::value ||
						  std::is_same<T, TextEditor>::value || std::is_same<T, RSLMultiLineEditor>::value,
                          "Invalid component type");
            return dynamic_cast<T*>(m_component.get());
		}
	};
	MyComponent();
	~MyComponent();
	void addLabel(String id, int x, int y, int w, int h, String initText);
	void addCodeEdit(String id, int x, int y, int w, int h, String inittext, Colour bgcolor, Colour txtcolor);
	void addMultiLineTextEdit(String id, int x, int y, int w, int h, String inittext, Colour bgcolor, Colour txtcolor);
	void addSingleLineTextEdit(String id, int x, int y, int w, int h, String inittext, Colour bgcolor, Colour txtcolor);
	void addButton(String id, int x, int y, int w, int h, String text, std::function<void(String, var)> callback);
	void addImageButton(String id, int x, int y, int w, int h, Image img, std::function<void(String, var)> callback);
	void addCheckBox(String id, int x, int y, int w, int h, String text, bool initstate,
		std::function<void(String, var)> callback = [](String, var) {});
	void addComboBox(String id, int x, int y, int w, int h, StringArray options, int initstate,
		std::function<void(String, var)> callback = [](String, var) {});
	void addRadioButtons(String id, int x, int y, int w, int h, 
		String title,StringArray options, int initstate);
	void addRadioButtons(String id,  int x, int y, int w, int h, std::initializer_list<RadioButtonsComponent::option_entry> options,
		String title, int initstate);
	void addPresetComponent(String id, int x, int y, int w, int h, String initialtext);
	template<typename T, typename... Args>
	inline void addCustomComponent(String id, int x, int y, int w, int h, Args... args)
	{
		static_assert(std::is_base_of<CustomComponentBase, T>::value, "Custom components must derive from CustomComponentBase!");
		ComponentEntry entry;
		entry.m_component = std::make_shared<T>(args...);
		entry.m_type = CT_Custom;
		m_components.push_back(entry);
		entry.m_component->setBounds(x, y, w, h);
		entry.m_component->setComponentID(id);
		addAndMakeVisible(entry.m_component.get());
	}
	var getComponentValue(String id, String propname = String());
	void setComponentValue(String id, var v, String propname = String());
	void setTipTextForComponent(String id, String tiptext);
	void setInfoLabelText(String text, bool redtext);

	ComponentEntry* getComponentEntryForID(String id);
	template<typename T=Component>
	T* getComponentFromID(String id)
	{
		static_assert(std::is_same<T, RSLCodeEditor>::value || std::is_same<T, PresetComponent>::value ||
			std::is_same<T, Component>::value || std::is_same<T, Slider>::value ||
			std::is_same<T, TextButton>::value || std::is_same<T, ImageButton>::value ||
			std::is_same<T, ToggleButton>::value || std::is_same<T, Label>::value ||
			std::is_same<T, RadioButtonsComponent>::value || std::is_same<T, ComboBox>::value ||
			std::is_same<T, TextEditor>::value || std::is_same<T, RSLMultiLineEditor>::value,
			"Invalid component type");
        for (auto& e : m_components)
			if (e.m_component->getComponentID() == id)
				return dynamic_cast<T*>(e.m_component.get());
		return nullptr;
	}
	
	ValueTree saveState();
	String loadState(ValueTree source);
	
	void timerCallback(int id) override;
	void buttonClicked(Button* button) override;
	void comboBoxChanged(ComboBox* combo) override;
	void resized() override;
private:
	std::vector<ComponentEntry> m_components;
	int m_infolabel_entry_index = -1;
	String getParentNameWithUnderScores();
	String m_last_detected_tip_component;
	Colour m_info_label_normal_colour;
	bool m_info_text_is_error = false;
	void updateInfoLabel();
};

class MyWindow : public ResizableWindow
{
public:
	MyWindow(String title, int w, int h, bool resizable, Colour bgcolor);
	~MyWindow();
	int getDesktopWindowStyleFlags() const override
	{
		if (isResizable()==true)
			return ComponentPeer::windowHasCloseButton | ComponentPeer::windowHasTitleBar | ComponentPeer::windowIsResizable | ComponentPeer::windowHasMinimiseButton;
		return ComponentPeer::windowHasCloseButton | ComponentPeer::windowHasTitleBar | ComponentPeer::windowHasMinimiseButton;
	}
	
	// Convenience methods that just redirect to the content component if it exists
	void addLabel(String id, int x, int y, int w, int h, String initText);
	void addCodeEdit(String id, int x, int y, int w, int h, String inittext, Colour bgcolor, Colour txtcolor);
	void addMultiLineTextEdit(String id, int x, int y, int w, int h, String inittext, Colour bgcolor, Colour txtcolor);
	void addSingleLineTextEdit(String id, int x, int y, int w, int h, String inittext, Colour bgcolor, Colour txtcolor);
	void addButton(String id, int x, int y, int w, int h, String text, std::function<void(String, var)> callback);
	void addImageButton(String id, int x, int y, int w, int h, Image img, std::function<void(String, var)> callback);
	void addImageButton(String id, int x, int y, Image img, std::function<void(String, var)> callback);
	void addCheckBox(String id, int x, int y, int w, int h, String text, bool initstate,
		std::function<void(String, var)> callback = [](String, var) {});
	void addComboBox(String id, int x, int y, int w, int h, StringArray options, int initstate,
		std::function<void(String, var)> callback = [](String, var) {});
	void addPresetComponent(String id, int x, int y, int w, int h, String initialtext);
	void addRadioButtons(String id, int x, int y, int w, int h,
		String title, StringArray options, int initstate);
	void addRadioButtons(String id, int x, int y, int w, int h, std::initializer_list<RadioButtonsComponent::option_entry> options,
		String title, int initstate);
	void setTipTextForComponent(String id, String tiptext);
	var getComponentValue(String id, String propname = String());
	void setComponentValue(String id, var v, String propname = String());

	void userTriedToCloseWindow() override;
	void visibilityChanged() override;
	bool keyPressed(const KeyPress& kp) override;
	ValueTree saveState();
	String loadState(ValueTree nvs);
	String getNameAsKey() const { return getName().replaceCharacter(' ', '_'); }
	
    template<typename T = Component>
	T* getComponentFromID(String id)
	{
        static_assert(std::is_same<T,RSLCodeEditor>::value || std::is_same<T,PresetComponent>::value ||
                      std::is_same<T,Component>::value || std::is_same<T,Slider>::value ||
                      std::is_same<T,TextButton>::value || std::is_same<T,ImageButton>::value ||
                      std::is_same<T,ToggleButton>::value || std::is_same<T,Label>::value ||
                      std::is_same<T,RadioButtonsComponent>::value || std::is_same<T,ComboBox>::value ||
					  std::is_same<T, TextEditor>::value || std::is_same<T, RSLMultiLineEditor>::value,
                      "Invalid component type");
        if (m_content != nullptr)
			return m_content->getComponentFromID<T>(id);
		return nullptr;
	}
	
	MyComponent* getMyComponent() { return m_content.get(); }
	std::function<void(MyWindow*)> CloseCallback;
	std::function<void(MyWindow*)> SaveCallback;
	static void deleteAllWindowInstances();
	static void initGuiIfNeeded();
	static void doGUIShutdown();
	void setDestroyOnClose(bool b) { m_destroy_on_close = b; }
protected:
	std::shared_ptr<MyComponent> m_content;
	bool m_destroy_on_close = false;
};



template<typename T>
inline T* make_window(String title, int wid, int h, bool resizable, Colour bgcolor)
{
	static_assert(std::is_base_of<MyWindow, T>::value, "Windows should derive from MyWindow!");
	MyWindow::initGuiIfNeeded();
	T* w = new T(title, wid, h, resizable, bgcolor);
	// This call order is important, the window should not be set visible
	// before adding it into the Reaper window hierarchy
	// Currently this only works for Windows, OS-X needs some really annoying special handling
	// not implemented yet
#ifdef WIN32
	w->addToDesktop(w->getDesktopWindowStyleFlags(), GetMainHwnd());
#else
	w->addToDesktop(w->getDesktopWindowStyleFlags(), 0);
    makeWindowFloatingPanel(w);
#endif
	w->setVisible(true);
	return w;
}

void saveWindowStateIntoGlobalSettings(String windowname,ValueTree vt);
ValueTree loadWindowStateFromGlobalSettings(String windowname);
void saveWindowStateIntoReaperProject(NamedValueSet nvs);

enum class ReaperFileLocation
{
	ReaperPluginDir,
	ReaperUserPluginDir,
	ReaperResourceDir,
	ReaperThemeDir,
	AbsolutePath
};

Image loadImage(String imgname, ReaperFileLocation location= ReaperFileLocation::ReaperResourceDir);

const auto bgcolor = Colour(0xffeacfd0);

// F must be a function that returns a non-empty String when an error occurs
template<typename F, typename... Args>
inline bool ExecuteWithErrorBox(F&& f, Args... args) {
	String err = f(args...);
	if (err.isEmpty() == false) {
		AlertWindow::showMessageBox(AlertWindow::WarningIcon, "Error", err);
		return false;
	}
	return true;
}

// F must be a function that returns a non-empty String when an error occurs
template<typename F, typename... Args>
inline bool ExecuteAndSetInfoTextOnError(MyWindow* w, F&& f, Args... args) {
	String err = f(args...);
	if (err.isEmpty() == false) {
		w->getMyComponent()->setInfoLabelText(err, true);
		return false;
	}
	return true;
}

class UserInputsComponent : public Component, public Button::Listener
{
public:
	struct field_t
	{
		field_t(String lbl, String initial)
		{
			m_label.setText(lbl, dontSendNotification);
			m_line_edit.setText(initial, dontSendNotification);
		}
		Label m_label;
		TextEditor m_line_edit;
	};
	UserInputsComponent()
	{
		addAndMakeVisible(&m_ok_button);
		m_ok_button.setButtonText("OK");
		m_ok_button.addListener(this);
	}
	std::function<void(UserInputsComponent*, int)> ButtonCallbackFunction;
	void setButtonText(int which, String txt)
	{
		if (which == 1)
			m_ok_button.setButtonText(txt);
		resized();
	}
	void buttonClicked(Button* but) override
	{
		if (ButtonCallbackFunction && but == &m_ok_button)
		{
			saveSettings();
			ButtonCallbackFunction(this, 1);
			return;
		}
	}

	void addEntry(String lbl, String initial)
	{
		auto entry = std::make_shared<field_t>(lbl, initial);
		addAndMakeVisible(&entry->m_label);
		addAndMakeVisible(&entry->m_line_edit);
		m_entries.push_back(entry);
	}
	int getMaximumLabelWidth(int minimumwidth)
	{
		int labelw = minimumwidth;
		Font labelfont = m_entries[0]->m_label.getFont();
		for (int i = 0; i < m_entries.size(); ++i)
		{
			int text_w = labelfont.getStringWidth(m_entries[i]->m_label.getText());
			labelw = std::max(text_w, labelw);
		}
		labelw = jlimit(minimumwidth, 1000, labelw);
		return labelw;
	}
	void resized() override
	{
		int entryh = 25;
		if (m_entries.size() > 0)
		{
			int labelw = getMaximumLabelWidth(150);
			for (int i = 0; i < m_entries.size(); ++i)
			{
				m_entries[i]->m_label.setBounds(1, i*entryh, labelw, entryh - 1);
				m_entries[i]->m_line_edit.setBounds(1 + labelw + 2, i*entryh, getWidth() - labelw - 4, entryh - 1);
			}
		}
		m_ok_button.changeWidthToFitText(24);
		m_ok_button.setTopLeftPosition(1, getHeight() - 25);
	}
	StringArray getResults()
	{
		StringArray results;
		for (auto& e : m_entries)
			results.add(e->m_line_edit.getText());
		return results;
	}
	String m_owner_name;
	void saveSettings()
	{
		auto r = getResults();
		ValueTree tree("state");
		for (int i = 0; i < r.size(); ++i)
		{
			tree.setProperty("text" + String(i), r[i], nullptr);
		}
		saveWindowStateIntoGlobalSettings(m_owner_name, tree);
	}
	void loadSettings()
	{
		ValueTree tree = loadWindowStateFromGlobalSettings(m_owner_name);
		if (tree.isValid() == false)
			return;
		int numprops = tree.getNumProperties();
		for (int i = 0; i < numprops; ++i)
		{
			if (i < m_entries.size())
			{
				String txt = tree.getProperty("text" + String(i));
				m_entries[i]->m_line_edit.setText(txt,dontSendNotification);
			}
		}
	}
private:
	std::vector<std::shared_ptr<field_t>> m_entries;
	TextButton m_ok_button;
	TextButton m_cancel_button;
};

class UserInputsWindow : public MyWindow
{
public:
	UserInputsWindow(String windowtitle, int w, int h, bool resizable, Colour bgcolor) :
		MyWindow(windowtitle, w, h, resizable, bgcolor)
	{
		m_comp = new UserInputsComponent;
		setContentOwned(m_comp, false);
		m_comp->m_owner_name = windowtitle;
	}
	
	UserInputsComponent* m_comp = nullptr;
private:

};

struct UIInputBox
{
  String name;
  String defval;
};

inline UserInputsWindow* ShowGenericUserInputs(String windowtitle, String but1txt, vector<UIInputBox> NameDefVal,
	std::function<void(UserInputsComponent*, int)> callback)
{
	auto dlg = make_window<UserInputsWindow>(windowtitle, 500, 400, true, Colours::grey);

	dlg->m_comp->ButtonCallbackFunction = callback;

	for (int i = 0; i < NameDefVal.size(); ++i)

	{
		String init_entry;
		if (i < NameDefVal.size())
			init_entry = NameDefVal[i].defval;
		dlg->m_comp->addEntry(NameDefVal[i].name, init_entry);
	}

	dlg->m_comp->loadSettings();
	dlg->m_comp->setButtonText(1, but1txt);
	dlg->setSize(500, NameDefVal.size() * 25 + 32);
	return dlg;
}
