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

class PROJECT
{
public:
	static double getEndTime();
	static double getGridDivision();
	static double getGridDivisionTime();
	static double getTempo();

	static String getFilePath();
	static String getDirectory();

	static int countMakersAndRegions();

	static File PROJECT::getUserFile(const String & title = "Choose a file", const String & fileFilter = "*.txt");
};


void msg(String s);
void COMMAND(int action, int flag = 0);
void COMMAND(const char* action, int flag = 0);

// project cursor functions
double SETCURSOR(double time, bool moveview = false, bool seekplay = false);
double GETCURSOR();

void UI();
void UNDO(String undostr = "ACTION ENDED EARLY", ReaProject* project = 0);
void VIEW();
void UPDATE();
void SET_ALL_ITEMS_OFFLINE();
void SET_ALL_ITEMS_ONLINE();
void SET_SELECTED_ITEMS_OFFLINE();
void SET_SELECTED_ITEMS_ONLINE();
void PASTE_ITEMS();
void REMOVE_ITEMS();
void GLUE_ITEMS();

namespace NUDGE {
	void START(double v, bool move_source = false);
}

// item functions
void UNSELECT_ITEMS(); // Unselect all items
MediaItem* SELECT_ITEM_UNDER_MOUSE();

int juceToReaperColor(const Colour & v);
Colour reaperToJuceColor(int v);

// [\\s\\S] replaces . for c++ regex since . does not match any character, only non-newline character

// chunk functions
static regex chunk_to_sections("(<[\\s\\S]*IID\\s\\d*\\n)(NAME[\\s\\S]*)(>\\n)");
static regex separate_take_chunks("TAKE(?: SEL)?\nNAME|NAME");
static regex split_take_from_sm_chunk("^(.*>\n)(SM.*)");
static regex get_rate_properties(".*?\\n(PLAYRATE .*?)\\n");
static regex split_rate_properties("PLAYRATE (.*?) (.*?) (.*?) (.*?) (.*?) (.*?)\\n");
static regex modify_rate_properties("^(.*?\\n)PLAYRATE (.*?) (.*?) (.*?) (.*?) (.*?) (.*?)(\\n.*)$");
void SplitTakeChunks(MediaItem* item, string chunk_c, string& header, string& footer, vector<string>& take_chunks, int& act_take_num);
struct TakeEnvMapStruct { regex r_search, r_replace; string defchunk; };
static map<string, TakeEnvMapStruct> TakeEnvMap =
{
	{ "Mute",{ (regex)"<MUTEENV\n", (regex)"<MUTEENV\n[\\s\\S]*?>\\s*", "<MUTEENV\nACT 1\nVIS 1 1 1\nLANEHEIGHT 0 0\nARM 1\nDEFSHAPE 1 -1 -1\nPT 0 1 1\n>\n" } },
	{ "Pitch",{ (regex)"<PITCHENV\n", (regex)"<PITCHENV\n[\\s\\S]*?>\\s*", "<PITCHENV\nACT 1\nVIS 1 1 1\nLANEHEIGHT 1 1\nARM 1\nDEFSHAPE 0 -1 -1\nPT 0 0 0\n>\n" } },
	{ "Pan",{ (regex)"><PANENV\n", (regex)"<PANENV\n[\\s\\S]*?>\\s*", "<PANENV\nACT 1\nVIS 1 1 1\nLANEHEIGHT 0 0\nARM 1\nDEFSHAPE 0 -1 -1\nPT 0 0 0\n>\n" } },
	{ "Volume",{ (regex)"<VOLENV\n", (regex)"<VOLENV\n[\\s\\S]*?>\\s*", "<VOLENV\nACT 1\nVIS 1 1 1\nLANEHEIGHT 0 0\nARM 1\nDEFSHAPE 0 -1 -1\nPT 0 1 0\n>\n" } }
};
static regex get_start_of_env_chunk("(>\\s*)");

// envelope functions
TrackEnvelope* ToggleTakeEnvelopeByName(MediaItem_Take* take, string env_name, bool off_on);

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
	virtual String getName() const = 0;
	// Set full name string of object also affecting the tag string portion
	virtual void setName(const String & v) = 0;

	virtual String GetPropertyStringFromKey(const String & key, bool use_value) const { return String(); }

	// Set a tag within the tag string
	String getTag(const String & key) const { return TagManager.getTag(key); }
	// Get a tag within the tag string
	void setTag(const String & key, const String & value)
	{
		TagManager.SetTag(key, value);
		setName(TagManager.getStringWithTags());
	}

	// Get name of object without tags
	String getNameNoTags() const { return TagManager.getNameNoTags(); }
	// Set name of object without affecting tags
	void setNameNoTags(const String & v)
	{
		TagManager.setStringNoTags(v);
		setName(TagManager.getStringWithTags());
	}

	// Get name of object only affecting the tag string portion
	String getTagString() const { return TagManager.getStringTagsOnly(); }
	// Set name of object only affecting the tag string portion
	void setTagString(const String & v)
	{
		TagManager.setStringWithTags(v);
		setName(TagManager.getStringWithTags());
	}

	// Remove a tag from the tag string
	void removeTag(const String & key)
	{
		TagManager.removeTag(key);
		setName(TagManager.getStringWithTags());
	}

	// Remove the entire tag string
	void removeAllTags()
	{
		TagManager.RemoveAllTags();
		setName(TagManager.getStringWithTags());
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
