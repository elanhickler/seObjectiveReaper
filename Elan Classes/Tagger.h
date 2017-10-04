#pragma once
#pragma warning(disable: 4592)

#include "../reaper plugin/reaper_plugin_functions.h"

/*
Tags
a  =  ~  articulation           legato
i  =  ~  interval and direction up12
n  = #/@ note                   C3 / 36
sn = #/@ starting note          C3 / 36
en = #/@ ending note            C4 / 48
v  = #/@ velocity layer         3  / MF
rr = #/@ repeitition            4  / d
*/

static vector<String> define_order({ "ls", "le", "a", "no", "d", "i", "n", "ns", "ne", "lv", "hv", "vl", "rr" });
static vector<String> define_uniques({ "ls", "le", "f", "r", "clip", "so", "sn", "v" });

class Tagger
{
private:
    String name;
    map<String, String> tagmap;
    MediaItem_Take* take;
    vector<String> order = define_order;
    vector<String> uniques = define_uniques;
    String KeySort(map<String, String> m) const;    
    void setup(String input);
public:
    Tagger() {}
    Tagger(String input);    

    /* Getters */
    // Return only tags.
    String TagsOnly() const;
    // Return string with tags.
    String WithTags() const;
    // Returns string without tags
    String NoTags() const;
    // Returns non-unique tags as string
    String ImportantTags() const;
    //Get tag string or value, returns "" on error
    String GetTag(const String & tag, char typechar = '@') const;

    /* Setters */
    // Set the tag string
    void TagsOnly(const String & tagString);    
    // Return string with tags.
    void WithTags(const String & input);
    // Set the non-tag string
    void NoTags(const String & input);
    // Set tag with value using int/double/string
    template<class t> void SetTag(String tag, t val)
    {
        tagmap[tag] = String(val);
    }
    // Remove tag
    void RemoveTag(const String& tag);
    // Remove a set of tags
    void RemoveTag(const vector<String>& TagList);
    // Remove all tags
    void RemoveAllTags();

    /* Boolean */
    // Check if tag exists
    bool TagExists(String tag) const;

    ///* Specific tag functions */

    //String GetNoteTag(bool search_tags = 1, bool use_regex = 1, int search_count = 1);
    ////Mode 0: Search for tags w,f,n,ne,ns, or use regex to find note
    ////Mode 1: Search for w,f tags
    ////Mode 2: Search for n,ne,ns tags
    //double GetFreqTag(int mode = 0, int search_count = 1);
    //void GetLegatoTags(int* start_out = nullptr, int* end_out = nullptr, int* interval_out = nullptr, int* dir_out = nullptr, bool shorthand = true);
    //int TransposeLegatoTags(int i);
    //void SetLegatoTags(int ns, int dir, int ne, bool shorthand = true);
};