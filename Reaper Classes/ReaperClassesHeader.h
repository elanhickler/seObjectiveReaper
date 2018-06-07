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

  void* m_data=nullptr;
  template<typename T>
  T* getDataAs() { return static_cast<T*>(m_data); }
};


class MyWindow;

template<class T, class U> void setOpt(T* parameter, U&& input)
{
    if (parameter != nullptr)  *parameter = std::move(input);
}

void msg(String s);
void COMMAND(int action, int flag = 0);
void COMMAND(const char* action, int flag = 0);

// project cursor functions
double SETCURSOR(double time, bool moveview = false, bool seekplay = false);
double GETCURSOR();

// project general functions
double GET_PROJECT_END_TIME();
void UI();
void UNDO(String undostr = "ACTION ENDED EARLY", ReaProject* project = 0);
void VIEW();
void UPDATE();
void SET_ALL_ITEMS_OFFLINE();
void SET_ALL_ITEMS_ONLINE();
void SET_SELECTED_ITEMS_OFFLINE();
void SET_SELECTED_ITEMS_ONLINE();

namespace NUDGE {
void START(double v, bool move_source = false);
}

// item functions
void UNSELECT_ITEMS(); // Unselect all items
MediaItem* SELECT_ITEM_UNDER_MOUSE();

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
private:
    bool _is_valid = true;
    virtual bool objectIsValid() const { return _is_valid; }
    void makeInvalid() { _is_valid = false; };
    void makeValid() { _is_valid = true; };

public:
    bool is_valid() const { return objectIsValid(); }
    virtual ~OBJECT_VALIDATES() {}
};

class OBJECT_NAMABLE
{
private:
    virtual String getObjectName() const { return String(); }
    virtual void setObjectName(const String & v) {}

public:
    // members
    Tagger TagManager;

    // functions
    virtual String GetPropertyStringFromKey(const String & key, bool use_value) const { return String(); }

    /* GETTER */

    String getTag(const String & key) const { return TagManager.GetTag(key); }
    // Get full name string of object including the tag string portion
    String getName() const { return getObjectName(); }
    // Get name of object without affecting tags
    String getNameNoTags() const { return TagManager.NoTags(); }
    // Get name of object only affecting the tag string portion
    String getNameTagsOnly() const { return TagManager.TagsOnly(); }

    /* SETTER */

    void removeAllTags() { TagManager.RemoveAllTags(); }
    void setTag(const String & key, const String & value) 
    {
      TagManager.SetTag(key, value); 
      setObjectName(TagManager.WithTags());
    }
    void removeTag(const String & key) 
    { 
      TagManager.RemoveTag(key);
      setObjectName(TagManager.WithTags());
    }
    // Set full name string of object also affecting the tag string portion
    void setName(const String & v) { setObjectName(v); }
    // Set name of object without affecting tags
    void setNameNoTags(const String & v) { TagManager.NoTags(v); }
    // Set name of object only affecting the tag string portion
    void setNameTagsOnly(const String & v) { TagManager.TagsOnly(v); }

    // boolean
    bool has_tag(const String & tag) const { return TagManager.TagExists(tag); }
};

class OBJECT_MOVABLE
{
private:
    virtual double getObjectStartPos() const { return 0.0; }
    virtual void setObjectStartPos(double v) {}

    virtual double getObjectEndPos() const { return getObjectStartPos() + getObjectLength(); }
    virtual void setObjectEndPos(double v) { setObjectLength(getObjectLength() - getObjectStartPos()); }

    virtual double getObjectLength() const { return 0.0; }
    virtual void setObjectLength(double v) {}

    virtual void setObjectPosition(double v) {}

    virtual int getObjectColor() const { return 0; }
    virtual void setObjectColor(int v) {}
public:
    operator RANGE() const { return range(); }

    // getters
    double startPos() const { return getObjectStartPos(); }
    double endPos() const { return getObjectEndPos(); }
    double length() const { return getObjectLength(); }
    int color() const { return getObjectColor(); }
    RANGE range() const { return { startPos(), endPos() }; }
    double position() { return getObjectStartPos(); }

    // setters
    void startPos(double v) { setObjectStartPos(v); }
    void endPos(double v) { setObjectEndPos(v); }
    void length(double v) { setObjectLength(v); }
    void color(int v) { setObjectColor(v); }
    void position(double v) { setObjectPosition(v); }
    void move(double v) { setObjectPosition(v + getObjectStartPos()); }
};

template <typename t> class LIST
{
    friend class MARKERLIST;
    friend class ITEMLIST;
    friend class ITEMGROUPLIST;
    friend class TAKELIST;
    friend class MIDINOTELIST;
private:
    bool do_sort = true;
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
    const t & back() const { return list.front(); }

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
    void sort() { if (do_sort) stable_sort(begin(), end(), [](const t & a, const t & b) { return a.startPos() < b.startPos(); }); }
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

    if (abs(list[m].start() - time) < abs(list[max(0, m-1)].start() - time))
        return list[m];
    else
        return list[max(0, m-1)];
}

template<typename t>
void LIST<t>::FilterByRange(RANGE range, bool must_be_completely_inside_range)
{
    LIST l;
    for (const auto & o : list)
    {
        if (o.startPos() < range.start())
            continue;
        else if (o.startPos() > range.end())
            break;

        if ((must_be_completely_inside_range && RANGE::is_inside(o, range))  ||
            (o.startPos() >= range.start() && o.startPos() < range.end()))
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
                l.push_back(list[i+counter]);

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

//class REAPEROBJECT
//{
//private:
//    enum class objtype { unassigned, envpt, envelope, take, item, track, marker };
//    objtype type = objtype::unassigned;
//
//public:
//    ENVPT * EnvPt = nullptr;
//    ENVELOPE * Envelope = nullptr;
//    TAKE * Take = nullptr;
//    ITEM * Item = nullptr;
//    TRACK * Track = nullptr;
//    MARKER * Marker = nullptr;
//
//    REAPEROBJECT(ENVPT & o) : EnvPt(&o) { type = objtype::envpt; }
//    REAPEROBJECT(ENVELOPE & o) : Envelope(&o) { type = objtype::envelope; }
//    REAPEROBJECT(TAKE & o) : Take(&o) { type = objtype::take; }
//    REAPEROBJECT(ITEM & o) : Item(&o) { type = objtype::item; }
//    REAPEROBJECT(TRACK & o) : Track(&o) { type = objtype::track; }
//    REAPEROBJECT(MARKER & o) : Marker(&o) { type = objtype::marker; }
//
//    operator ENVPT*() const { return EnvPt; }
//    operator ENVELOPE*() const { return Envelope; }
//    operator TAKE*() const { return Take; }
//    operator ITEM*() const { return Item; }
//    operator TRACK*() const { return Track; }
//    operator MARKER*() const { return Marker; }
//
//    bool is_ENVPT() { return EnvPt != nullptr; }
//    bool is_ENVELOPE() { return Envelope != nullptr; }
//    bool is_TAKE() { return Take != nullptr; }
//    bool is_ITEM() { return Item != nullptr; }
//    bool is_TRACK() { return Track != nullptr; }
//    bool is_MARKER() { return Marker != nullptr; }    
//
//    double startPos() const
//    {
//        switch (type)
//        {
//        case objtype::envpt: return EnvPt->startPos();
//        case objtype::take: return Take->startPos();
//        case objtype::item: return Item->startPos();
//        case objtype::marker: return Marker->startPos();
//        }
//        return 0.0;
//    }
//};

//class REAPEROBJECTLIST
//{
//public:
//    vector<REAPEROBJECT> list;
//
//    REAPEROBJECTLIST() {}
//
//    template <class t> collect(t & objectlist)
//    {
//        for (auto & o : objectlist)
//            list.push_back(REAPEROBJECT(o));
//    }
//
//    // iteration
//    vector<REAPEROBJECT>::iterator begin() { return list.begin(); }
//    vector<REAPEROBJECT>::iterator end() { return list.end(); }
//    vector<REAPEROBJECT>::const_iterator begin() const { return list.cbegin(); }
//    vector<REAPEROBJECT>::const_iterator end() const { return list.cend(); }
//    REAPEROBJECT & back() { return list.back(); }
//    const REAPEROBJECT & back() const { return list.back(); }
//
//    void sort() { stable_sort(begin(), end(), [](const REAPEROBJECT & a, const REAPEROBJECT & b) { return a.startPos() < b.startPos(); }); }
//};
