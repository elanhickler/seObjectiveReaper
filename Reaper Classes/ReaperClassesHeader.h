#pragma once
#pragma warning(disable: 4267) // size_t to int

#include "JuceHeader.h"

#include "../Elan Classes/ElanClassesHeader.h"
#include <set>
#include <regex>

using std::function;
using std::regex;
using std::match_results;
using std::sregex_token_iterator;

#include "../reaper plugin/reaper_plugin_functions.h"

using std::set;
using std::string;
using std::vector;
using std::min;
using std::max;
#pragma warning(disable: 4244) // conversion possible loss of data

enum toggle_state { CannotToggle, ToggleOff, ToggleOn };

class action_entry
{ //class for registering actions
public:
	action_entry(string description, string idstring, toggle_state togst, std::function<void(action_entry&)> func);
	action_entry(const action_entry&) = delete; // prevent copying
	action_entry& operator=(const action_entry&) = delete; // prevent copying
	action_entry(action_entry&&) = delete; // prevent moving
	action_entry& operator=(action_entry&&) = delete; // prevent moving

	int m_command_id = 0;
	gaccel_register_t m_accel_reg;
	function<void(action_entry&)> m_func;
	string m_desc;
	string m_id_string;
	toggle_state m_togglestate = CannotToggle;

	void* m_data = nullptr;
	template<typename T>
	T* getDataAs() { return static_cast<T*>(m_data); }
};

class MyWindow;

template<class T, class U> void setOpt(T* parameter, U&& input)
{
	if (parameter != nullptr)  *parameter = std::move(input);
}

class CONSOLE
{
public:
	static void log(const String& text)
	{
		ShowConsoleMsg(String(text + "\n").toRawUTF8());
	}
	static void clear(const String& text = "")
	{
		ShowConsoleMsg("");
		ShowConsoleMsg(String(text + "\n").toRawUTF8());
	}
};

class COMMAND
{
public:
	static void pasteItems() { COMMAND(40058); }
	static void removeItems() { COMMAND(40006); }
	static void glueItems() { COMMAND(40362); }
	static void commonNormalizeItems() { COMMAND(40254); }
	static void unselectAllItemsAndSelectItemUnderMouse() { COMMAND(40528); }
	static void crossfadeSelectedOverlappingItems() { COMMAND(41059); }
	static void gotoNextMarkerOrPojectEnd() { COMMAND(40173); };
	static void selectItemUnderMouse() { COMMAND(40528); }
	static void setCursorToMouse() { COMMAND(40514); }
	static void openInEditor() { COMMAND(40109); }

	COMMAND(int action, int flag = 0) { Main_OnCommand(action, flag); }
	COMMAND(String action, int flag = 0) { Main_OnCommand(NamedCommandLookup(action.toRawUTF8()), flag); }
};

class NUDGE
{
public:
	enum what {
		position,
		start, //leftTrim,
		positionKeepingEnd, //leftEdge,
		end, //rightTrim or rightEdge,
		contents,
		duplicate,
		editCursor,
		invalid
	};

	enum units
	{
		ms,
		seconds,
		grid,
		samples = 17,
		frames,
		pixels,
	};

	static void apply(what w, double amount, units u = units::seconds)
	{
		ApplyNudge(0, 0, w, u, amount, false, 0);
	}
};

class MARKER;
class ITEM;

class PROJECT
{
protected:
	static vector<MediaItem*> savedItems;
	static double saved_cursor_position;
	static bool view_is_being_saved;
	static double global_save_view_start;
	static double global_save_view_end;

public:
	static double getEndTime();
	static double getGridDivision();
	static double getGridDivisionTime();
	static double getTempo();
	static void getTimeSignature(int& numerator, int& denominator);
	static double getBarSeconds()
	{
		return TimeMap_GetMeasureInfo(nullptr, 1, nullptr, nullptr, nullptr, nullptr, nullptr);
	}

	static String getFilePath();
	static String getDirectory();
	static String getName();

	static int countMakersAndRegions();
	static int countSelectedItems() { return CountSelectedMediaItems(0); }
	static int countItems();
	static int countTracks();
	static int countSelectedTracks();

	static void selectItem(MediaItem* itemPtr) { SetMediaItemInfo_Value(itemPtr, "B_UISEL", true); }
	static void selectItemsUnderCursor();
	static void selectNextItem();
	static void selectPreviousItem();

	static void unselectItem(MediaItem* itemPtr);
	static void unselectAllItems() { COMMAND(40289); }

	static void setSelectedItemsOffline() { COMMAND(40440); }
	static void setSelectedItemsOnline() { COMMAND(40439); }
	static void setAllItemsOffline() { COMMAND(40100); }
	static void setAllItemsOnline() { COMMAND(40101); }

	static void saveItemSelection();
	static void loadItemSelection();

	static void saveCursor();
	static void loadCursor();

	static void saveView();
	static void loadView();
	static void scrollView(double pos, double view_factor);

	static void play() { COMMAND(1007); }
	static void play(double time) { SetEditCurPos(time, false, true); COMMAND(1007); }
	static void playSelectedItem() { COMMAND("_XENAKIOS_TIMERTEST1"); }
	static void stop() { COMMAND(1016); }

	static double getMousePosition();
	static double setCursor(double time, bool moveview = false, bool seekplay = false);
	static double getCursor();

	static String getClipboardFile(String clipboardName = "");

	static void setClipboard(const String& v)
	{
#ifdef WIN32
		if (OpenClipboard(nullptr))
		{
			HGLOBAL clipbuffer;
			char* buffer;
			EmptyClipboard();
			clipbuffer = GlobalAlloc(GMEM_DDESHARE, v.length() + 1);
			buffer = (char*)GlobalLock(clipbuffer);
			strcpy(buffer, LPCSTR(v.toRawUTF8()));
			GlobalUnlock(clipbuffer);
			SetClipboardData(CF_TEXT, clipbuffer);
			CloseClipboard();
		}
#endif
	}

	static String getClipboard()
	{
#ifdef WIN32
		char* buffer;
		String ret;
		if (OpenClipboard(nullptr))
		{
			buffer = (char*)GetClipboardData(CF_TEXT);
			ret = buffer;
		}

		CloseClipboard();

		return ret;
#endif
	}
};

void UI();
void UNDO(String undostr = "ACTION ENDED EARLY", ReaProject* project = 0);
void UPDATE();
void UNDO_WITH_ERROR(String undostr, ReaProject* project = 0);

int juceToReaperColor(const Colour & v);
Colour reaperToJuceColor(int v);
static vector<int> notecolors = { 0xa6cee3,0x1f78b4,0xb2df8a,0x33a02c,0xffff99,0xfb9a99,0xe31a1c,0xfdbf6f,0xff7f00,0xcab2d6,0x6a3d9a,0xb15928 };

// [\\s\\S] replaces . for c++ regex since . does not match newline characters

// chunk functions
static regex chunk_to_sections("(<[\\s\\S]*IID\\s\\d*\\n)(NAME[\\s\\S]*)(>\\n)");
static regex separate_take_chunks("TAKE(?: SEL)?\nNAME|NAME");
static regex split_take_from_sm_chunk("^(.*>\n)(SM.*)");
static regex get_rate_properties(".*?\\n(PLAYRATE .*?)\\n");
static regex split_rate_properties("PLAYRATE (.*?) (.*?) (.*?) (.*?) (.*?) (.*?)\\n");
static regex modify_rate_properties("^(.*?\\n)PLAYRATE (.*?) (.*?) (.*?) (.*?) (.*?) (.*?)(\\n.*)$");

class AUDIODATA
{
public:
	WavAudioFile * cues;

	AUDIODATA() {}

	vector<double>& operator[](int i) { return data[i]; }
	vector<double> operator[](int i) const { return data[i]; }

	AUDIODATA(const vector<vector<double>> & multichannelAudio, int sampleRate, int bitDepth);

	AUDIODATA(const vector<vector<float>> & multichannelAudio, int sampleRate, int bitDepth);

	AUDIODATA(const vector<double> & singleChannelAudio, int sampleRate, int bitDepth);

	AUDIODATA(const vector<float> & singleChannelAudio, int sampleRate, int bitDepth);

	AUDIODATA(PCM_source* source);

	AUDIODATA(const File & file);

	void setSource(PCM_source* source);

	void setSource(const File & file);

	void setSource(const vector<vector<double>> multichannelAudio, int sampleRate, int bitDepth);

	void writeToFile(const File & file) const;

	void collectCues();
	Array<WavAudioFile::CuePoint> getCuePoints();
	Array<WavAudioFile::Region> getCueRegions();
	Array<WavAudioFile::Loop> getLoops();
	void writeCues();

	// getters
	File getFile() const { return file; }
	double getSample(int channel, int sample) { return data[channel][sample]; }
	vector<double> & getChannel(int channel) { return data[channel]; }
	vector<vector<double>> & getData() { return data; }

	//// separate channels with '|', example 1|2|4, which will return a mono signal mixing channel 1, 2, and 4. NOTE: channels are 1-base. If blank, return full mono mixdown. Optional seconds limits the amount of signal to return.
	//template <typename t> vector<t> getMonoMixdown(double seconds = 0)
	//{
	//	int numFrames = seconds > 0 ? min<int>(getSampleRate() * seconds, getNumFrames()) : getNumFrames();

	//	vector<t> signal;
	//	signal.reserve(numFrames);
	//	for (int c = 0; c < getNumChannels(); ++c)
	//		for (int s = 0; s < numFrames; ++s)
	//			signal.push_back(getSample(c, s));

	//	return std::move(signal);
	//}

	// separate channels with '|', example 1|2|4, which will return a mono signal mixing channel 1, 2, and 4.
	// NOTE: channels are 1-base. If blank, return full mono mixdown.
	// Optional seconds limits the amount of signal to return.
	template <typename t> vector<t> getMixdownViaChannelString(String channelString = "", double seconds = 0)
	{
		vector<String> channelListString = STR::splitToVector(channelString, "|");
		vector<int> channelList;

		for (const auto& s : channelListString)
		{
			if (!STR::isInt(s))
			{
				jassertfalse; // string is not a number
				continue;
			}

			int value = s.getIntValue() - 1; // 1 base to 0 base

			if (isPositiveAndBelow(value, getNumChannels()))
				VectorHelper::pushUnique<int>(channelList, value);
			else
				jassertfalse; // invalid channel
		}

		if (channelList.empty()) // populate with all channels
			for (int c = 0; c < getNumChannels(); ++c)
				channelList.push_back(c);
			//return getMonoMixdown<t>(seconds);

		int sr = getSampleRate();

		int numFrames = seconds > 0 ? min<int>(getSampleRate() * seconds, getNumFrames()) : getNumFrames();

		vector<t> ret;
		ret.reserve(numFrames);
		for (int i = 0; i < numFrames; ++i)
		{
			t value = t(0);

			for (auto c : channelList)
				value += getSample(c, i);

			ret.push_back(value / double(channelList.size()));
		}

		return std::move(ret);
	}

	int getNumSamples() const { return samples; }
	int getNumChannels() const { return channels; }
	int getNumFrames() const { return frames; }
	int getSampleRate() const { return srate; }
	int getBitDepth() const { return bitdepth; }
	double getLength() const { return length; }

	// setters
	void setSampleRate(int v);
	void setBitDepth(int v);
	void setNumFrames(int v);
	void setNumChannels(int v);
	void setSample(int channel, int frame, double value);

	void clear()
	{
		data.clear();
		file = File();
		srate = 0;
		bitdepth = 0;
		frames = 0;
		samples = 0;
		channels = 0;
		length = 0;
	}

protected:
	File file;
	int srate = 0;
	int bitdepth = 0;
	int frames = 0;
	int samples = 0;
	int channels = 0;
	double length = 0;
	vector<vector<double>> data;

	AudioSampleBuffer convertToAudioSampleBuffer() const;

	template <typename t1, typename t2> vector<vector<t1>> convertAudioType(const vector<vector<t2>> & type2Audio) const
	{
		vector<vector<t1>> type1Audio;

		type1Audio.reserve(type2Audio.size());
		for (int ch = 0; ch < type2Audio.size(); ++ch)
			type1Audio.push_back({ type2Audio[ch].begin(), type2Audio[ch].end() });

		return std::move(type1Audio);
	}

	template <typename t1, typename t2> vector<vector<t1>> convertAudioType(const vector<t2> & type2Audio) const
	{
		vector<vector<t1>> type1Audio;

		type1Audio.push_back({ type2Audio.begin(), type2Audio.end() });

		return std::move(type1Audio);
	}
};

// Class used to simply add a boolean for if the object is valid or not
class OBJECT_VALIDATES
{
	friend class MARKERLIST;
public:
	virtual bool isValid() const { return is_valid; }
	virtual ~OBJECT_VALIDATES() {}

protected:
	bool is_valid = true;
	void makeInvalid() { is_valid = false; };
	void makeValid() { is_valid = true; };
};

class OBJECT_NAMABLE
{
public:
	Tagger TagManager;
protected:
  // members
	// Call this function in the derived class's constructor like this: OBJECT_NAMABLE::initialize()
	// Make sure you call this AFTER the object actually has a name.
	void initialize() { TagManager.setString(getObjectName()); }
	// Override this to provide a way for retreiving name string from the object, protect this function
	virtual String getObjectName() const = 0;
	// Override this to provide a way for applying the name string to the object, protect this function
	virtual void setObjectName(const String & v) = 0;

public:
 	virtual ~OBJECT_NAMABLE(){}
  
	// Get the object's name including tags
	String getName() const
	{
		return TagManager.getString();
	}
	// Set the object's full name string which will also overwrite tags
	void setName(const String & v)
	{
		TagManager.setString(v);
		setObjectName(v);
	}

	virtual String GetPropertyStringFromKey(const String & key, bool use_value) const { return {}; }

	// Set a tag within the tag string
	String getTag(const String & key) const { return TagManager.getTag(key); }
	// Get a tag within the tag string
	void setTag(const String & key, const String & value)
	{
		TagManager.setTag(key, value);
		setObjectName(TagManager.getString());
	}

	// Get name of object without tags
	String getNameNoTags() const { return TagManager.getNameString(); }
	// Set name of object without affecting tags
	void setNameNoTags(const String & v)
	{
		TagManager.setNameString(v);
		setObjectName(TagManager.getString());
	}

	// Get name of object only affecting the tag string portion
	String getNameTagsOnly() const { return TagManager.getTagString(); }
	// Set name of object only affecting the tag string portion
	void setNameTagsOnly(const String & v)
	{
		TagManager.setString(v);
		setObjectName(TagManager.getString());
	}

	// Remove a tag from the tag string
	void removeTag(const String & key)
	{
		TagManager.removeTag(key);
		setObjectName(TagManager.getString());
	}

	// Remove the entire tag string
	void removeAllTags()
	{
		TagManager.removeAllTags();
		setObjectName(TagManager.getNameString());
	}

	// boolean
	bool hasTag(const String & tag) const { return TagManager.hasTag(tag); }
};

// Override getStart/getEnd functions and optionally setPosition/Start/End/Length, as wll as optionally get/set Color.
class OBJECT_MOVABLE
{
public:
  virtual ~OBJECT_MOVABLE(){}
  
	operator RANGE() { return range(); }

	// getters
	virtual double getStart() const = 0;
	virtual double getEnd() const = 0;
	virtual double getLength() const { return getEnd() - getStart(); }
	virtual Colour getColor() const { return {}; }

	RANGE range() const { return { getStart(), getEnd() }; }

	// setters
	virtual void setPosition(double v) { }
	virtual void setStart(double v) { }
	virtual void setEnd(double v) { }
	virtual void setLength(double v) { }
	virtual void setColor(Colour v) { }

	void move(double v) { setPosition(v + getStart()); }
};

template <typename t> class LIST
{
public:
	// members
	vector<t> list;

	// operators
	t & operator[](size_t i) { return list[i]; }
	const t & operator[](size_t i) const { return list[i]; }

	// properties
	size_t size() const { return list.size(); }

	// iteration
	typename vector<t>::iterator begin() { return list.begin(); }
	typename vector<t>::iterator end() { return list.end(); }
	typename vector<t>::const_iterator begin() const { return list.cbegin(); }
	typename vector<t>::const_iterator end() const { return list.cend(); }
	t & front() { return list.front(); }
	const t & front() const { return list.front(); }
	t & back() { return list.back(); }
	const t & back() const { return list.back(); }

	// boolean
	bool empty() const { return list.empty(); }
	bool has(const t & obj) const
	{
		for (const t & i : list)
			if (obj == i)
				return true;
		return false;
	}

	// vector functions
	void reserve(size_t s) { list.reserve(s); }
	void push_back(const t & o) { list.push_back(o); }
	void pop_back() { list.pop_back(); }
	//void insert(typename vector<t>::iterator position, typename vector<t>::iterator first, typename vector<t>::iterator last) { insert(position, first, last); }
	void append(const LIST & l) { list.insert(list.end(), l.begin(), l.end()); }
	void clear() { list.clear(); }
	void sort() { if (do_sort) stable_sort(begin(), end(), [](const t & a, const t & b) { return a.getStart() < b.getStart(); }); }
	void resize(size_t size) { list.resize(size); }
	//void erase(vector<t>::const_iterator first, vector<t>::const_iterator last) { list.erase(first, last); }

	// search functions
	t SearchRange(RANGE r, bool find_near_start);
	t SearchAtOrBeforeTime(double time);
	t SearchNearestToTime(double time);

	// filter functions
	void FilterByRange(RANGE range, bool must_be_completely_inside_range = false);
	void FilterByPattern(char do_not_filter, const String & pattern, bool full_pattern_must_complete);

	// setters
	void disableSort() { do_sort = false; }

protected:
	bool do_sort = true;
};

#include "ActionEntry.h"
#include "StretchMarker.h"
#include "Env.h"
#include "Take.h"
#include "Item.h"
#include "Track.h"
#include "Marker.h"

// search functions
template<typename t> t LIST<t>::SearchRange(RANGE r, bool find_near_start)
{
	int i = 0;

	if (find_near_start)
	{
		while (list.size() < i && list[i].start() < r.start()) ++i;
	}
	else
	{
		while (list.size() < i && list[i].start() < r.end()) ++i;
		--i;
	}

	if (r.is_touching(list[i].start()))
		return list[i];

  return {};
}

template<typename t>
t LIST<t>::SearchAtOrBeforeTime(double time)
{
	int i = 0;
	while (i < list.size() && list[i].start() <= time) ++i;

	if (i != 0) --i;

	if (list[i].start() <= time)
		return list[i];
	else
    return {};
}

template<typename t>
t LIST<t>::SearchNearestToTime(double time)
{
	if (!list.size())
    return MARKER();
	if (list.size() == 1)
    return list[0];

	int m = 0;
	while (m < list.size() && list[m].start() < time)
    ++m;

	if (abs(list[m].start() - time) < abs(list[max(0, m - 1)].start() - time))
		return list[m];
	else
		return list[max(0, m - 1)];
}

template<typename t>
void LIST<t>::FilterByRange(RANGE range, bool must_be_completely_inside_range)
{
	LIST l;
	for (const auto& o : list)
	{
		if (o.getStart() < range.start())
			continue;
		else if (o.getStart() > range.end())
			break;

		if ((must_be_completely_inside_range && RANGE::is_inside(o, range)) ||
			(o.getStart() >= range.start() && o.getStart() < range.end()))
			l.push_back(o);
	}

	list = std::move(l.list);
}

template<typename t>
void LIST<t>::FilterByPattern(char do_not_filter, const String & pattern, bool full_pattern_must_complete)
{
	LIST l;
	int counter = 0;
	const int len = pattern.length();
	const int do_full_pattern = full_pattern_must_complete ? len : 0;

	while (counter + do_full_pattern < list.size())
	{
		for (int i = 0; i < len; ++i)
			if (pattern[i] == do_not_filter)
				l.push_back(list[i + counter]);

		counter += len;
	}

	list = std::move(l.list);
}

//// Media Item functions
//double GetNearestItemStartTime(double time, TRACKLIST & TrackListIn);
//double GetNearestItemEndTime(double time, TRACKLIST & TrackListIn);
//ITEM GetNextItemToTimeBasedOnStart(double time, ITEMLIST & ItemList, int starting_idx = 0);
//ITEM GetNearestItemToTimeBasedOnStart(double time, ITEMLIST & ItemListIn);
//ITEM GetNearestItemToTimeBasedOnEnd(double time, ITEMLIST & ItemListIn);
//double GetNearestItemStartTime(double time, const TRACKLIST & TrackList);
//RANGE GetRangeOfSelectedItems(const TRACKLIST & TrackList);
//ITEMGROUPLIST GetGroupsOfItems(const vector<RANGE> & list, TRACKLIST & TrackListIn);
//
//// Project Marker functions
//MARKERLIST GetMarkersInRange(RANGE range, MARKERLIST & MarkerListIn, bool createGhostMarkers);
////MARKER GetPreviousMarkerOrProjectStart(double time);
//MARKER GetNextMarkerOrProjectEnd(double time, MARKERLIST & MarkerListIn, int starting_idx = 0);
