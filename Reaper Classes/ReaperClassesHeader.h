#pragma once
#pragma warning(disable: 4267) // size_t to int

#include "JuceHeader.h"

#include "../Elan Classes/ElanClassesHeader.h"
#include <set>
#include <regex>

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
	CONSOLE(String text)
	{
		ShowConsoleMsg(text.toRawUTF8());
	}
	static void clear()
	{
		ShowConsoleMsg("");
	}
};

class COMMAND
{
public:
	static void pasteItems() { COMMAND(40058); }
	static void removeItems() { COMMAND(40006); }
	static void glueItems() { COMMAND(40362); }
	void unselectAllItemsAndSelectItemUnderMouse() { COMMAND(40528); }

	COMMAND(int action, int flag = 0) { Main_OnCommand(action, flag); }
	COMMAND(const char* action, int flag = 0) { Main_OnCommand(NamedCommandLookup(action), flag); }
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

	static String getFilePath();
	static String getDirectory();
	static String getName();

	static int countMakersAndRegions();
	static int countSelectedItems() { return CountSelectedMediaItems(0); }
	static int countItems();
	static int countTracks();
	static int countSelectedTracks();

	static void selectItem(MediaItem* itemPtr) { SetMediaItemInfo_Value(itemPtr, "B_UISEL", true); }
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

	static double setCursor(double time, bool moveview = false, bool seekplay = false);
	static double getCursor();
};

void UI();
void UNDO(String undostr = "ACTION ENDED EARLY", ReaProject* project = 0);
void UPDATE();

int juceToReaperColor(const Colour & v);
Colour reaperToJuceColor(int v);

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
		int srate = 0;
		int bitdepth = 0;
		int frames = 0;
		int samples = 0;
		int channels = 0;
		double length = 0;
	}

protected:
	File file;
	int srate;
	int bitdepth;
	int frames;
	int samples;
	int channels;
	double length;
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
protected:
	// Call this function in the derived class's constructor like this: OBJECT_NAMABLE::initialize()
	// Make sure you call this AFTER the object actually has a name.
	void initialize() { TagManager.setStringWithTags(getObjectName()); }
	// Override this to provide a way for retreiving name string from the object, protect this function
	virtual String getObjectName() const = 0;
	// Override this to provide a way for applying the name string to the object, protect this function
	virtual void setObjectName(const String & v) = 0;

public:
	// Get the object's name including tags
	String getName() const
	{
		return TagManager.getStringWithTags();
	}
	// Set the object's full name string which will also overwrite tags
	void setName(const String & v)
	{
		TagManager.setStringWithTags(v);
		setObjectName(v);
	}

	virtual String GetPropertyStringFromKey(const String & key, bool use_value) const { return {}; }

	// Set a tag within the tag string
	String getTag(const String & key) const { return TagManager.getTag(key); }
	// Get a tag within the tag string
	void setTag(const String & key, const String & value)
	{
		TagManager.SetTag(key, value);
		setObjectName(TagManager.getStringWithTags());
	}

	// Get name of object without tags
	String getNameNoTags() const { return TagManager.getNameNoTags(); }
	// Set name of object without affecting tags
	void setNameNoTags(const String & v)
	{
		TagManager.setStringNoTags(v);
		setObjectName(TagManager.getStringWithTags());
	}

	// Get name of object only affecting the tag string portion
	String getNameTagsOnly() const { return TagManager.getStringTagsOnly(); }
	// Set name of object only affecting the tag string portion
	void setNameTagsOnly(const String & v)
	{
		TagManager.setStringWithTags(v);
		setObjectName(TagManager.getStringWithTags());
	}

	// Remove a tag from the tag string
	void removeTag(const String & key)
	{
		TagManager.removeTag(key);
		setObjectName(TagManager.getStringWithTags());
	}

	// Remove the entire tag string
	void removeAllTags()
	{
		TagManager.RemoveAllTags();
		setObjectName(TagManager.getNameNoTags());
	}

	// boolean
	bool hasTag(const String & tag) const { return TagManager.tagExists(tag); }

protected:
	// members
	Tagger TagManager;
};

class OBJECT_MOVABLE
{
public:
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

	return InvalidTake;
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
		return InvalidTake;
}

template<typename t>
t LIST<t>::SearchNearestToTime(double time)
{
	if (!list.size()) return MARKER();
	if (list.size() == 1) return list[0];

	int m = 0;
	while (m < list.size() && list[m].start() < time) ++m;

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
	int mod = 0;
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
