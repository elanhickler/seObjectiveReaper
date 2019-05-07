#include <vector>
#include <map>

#include "JuceHeader.h"

using std::vector;
using std::map;
using STR::splitToVector;

#include "Tagger.h"

void Tagger::setup(String input)
{
    tagmap.clear();

    vector<String> tempvect = STR::split(input, '`', 1, "");
    name = tempvect[0];

    for (int i = 1; i < tempvect.size(); ++i)
    {
        vector<String> temp = splitToVector(tempvect[i], '=');
        if (temp.size() < 2) continue;
        tagmap[temp[0]] = temp[1];
    }
}

Tagger::Tagger(String input)
{
    setup(input);
}

// Return full name and tags.
String Tagger::KeySort(map<String, String> m) const
{
    String s;
    for (const auto& it : order)
    {
        auto needle = m.find(it);
        if (needle != m.end())
        {
            s += "`" + needle->first + "=" + needle->second;
            m.erase(needle);
        }
    }
    for (auto&& it : m) s += "`" + it.first + "=" + it.second;
    return s;
}

String Tagger::getStringTagsOnly() const
{
    return KeySort(tagmap);
}
String Tagger::getStringWithTags() const
{
    return getNameNoTags() + getStringTagsOnly();
}
String Tagger::getImportantTagString() const
{
    auto temp_map = tagmap;
    for (const auto& tag : uniques) temp_map.erase(tag);
    return name + KeySort(temp_map);
}

String Tagger::getNameNoTags() const
{
    return name;
}
void Tagger::setStringWithTags(const String & input)
{
    setup(input);
}

void Tagger::setStringNoTags(const String & input)
{
    name = input;
}

//returns "" on error
bool Tagger::tagExists(String tag) const
{
    return tagmap.find(tag) != tagmap.end();
}
String Tagger::getTag(const String & tag, char typechar) const
{
    auto it = tagmap.find(tag);

    if (it == tagmap.end()) 
      return "";

    return it->second;
}
void Tagger::removeTag(const String& tag)
{
    tagmap.erase(tag);
}
void Tagger::RemoveAllTags()
{
    tagmap.clear();
}
void Tagger::removeTags(const vector<String>& TagList)
{
    for (const auto& tag : TagList)
			removeTag(tag);
}

//String GetNoteTag(bool search_tags, bool use_regex, int search_count) { //todo: support legato note tag style
//	if (search_tags) {
//		if (tagExists("n")) return getTag("n");
//		else if (tagExists("ne")) return getTag("ne");
//		else if (tagExists("ns")) return getTag("ne");
//	}
//	else if (use_regex) {
//		regex expr(".*([a-gA-G]#?(?:-2|-1|[0-8])).*");
//		int found = 0;
//		for (boost::sregex_iterator it(name.begin(), name.end(), expr), itEnd; it != itEnd; ++it) {
//			if ((*it)[1] != "") ++found;
//			String t = (*it)[1];
//			if (found >= search_count) return (*it)[1];
//		}
//	}
//
//	return "";
//}
//
////Mode 0: Search for tags w,f,n,ne,ns, or use regex to find note
////Mode 1: Search for w,f tags
////Mode 2: Search for n,ne,ns tags
//double GetFreqTag(int mode, int search_count) {
//	int h = 1;
//	double f = 0;
//	String notename = "";
//
//	if (tagExists("h"))	h = stoi(getTag("h"));
//
//	if (mode == 0 || mode == 1) {
//		if (tagExists("w")) f = stod(getTag("w"));
//		else if (tagExists("f")) f = stod(getTag("f"));
//	}
//
//	if (f == 0 && (mode == 0 || mode == 2))
//		f = Midi::ToFreq(GetNoteTag(mode == 2 || mode == 0, mode == 0, search_count));
//
//	return f*h;
//}
//
//regex split_legato_str("^([A-G]#?(?:-2|-1|[0-8])|\\d{ 1,3 }).?([A-G]#?(?:-2|-1|[0-8])|\\d{ 1,3 })?$", regex_flag_e::icase);
//void evaluate_legato_str(String legstr, String& note_start, String& note_end) {
//	boost::match_results<std::String::const_iterator> m;
//	regex_search(legstr, m, split_legato_str);
//	note_start = m[1];
//	note_end = m[2];
//}
//void evaluate_legato_str(String legstr, int& note_start, int& note_end) {
//	String ns, ne;
//	evaluate_legato_str(legstr, ns, ne);
//	note_start = Midi::ToNum(ns);
//	note_end = Midi::ToNum(ne);
//}
//
//// ACCEPTABLE MATCHES:
//// u, up
//regex match_up_str("^up?$", boost::regex_constants::icase);
//// d, dn, down
//regex match_down_str("^d(?:ow)?n?$", boost::regex_constants::icase);
//// r, rp, rep, repeat, repluck, rb, reb, rebow, rs, res, restroke // todo: make user settable
//regex match_rep_str("^re?(?:p(?:eat|luck)?|b(?:ow)?|s(?:troke)?)?$", boost::regex_constants::icase);
//// returns "" on error
//String evaluate_dir_str(const String& dir) {
//	if (regex_match(dir, match_up_str)) return "u";
//	if (regex_match(dir, match_down_str)) return "d";
//	if (regex_match(dir, match_rep_str)) return "r";
//	return "";
//}
//String evaluate_dir_str(char dir) {
//	return evaluate_dir_str(ToS(dir));
//}

//int TransposeLegatoTags(int i) { //todo: unconvolute, make better consolidated functions, tag manager needs hard-coded tags, inefficient to set legato tags twice
//	int ns, ne;
//	int dir;
//    int ret;
//
//	GetLegatoTags(&ns, &ne, nullptr, &dir);
//
//	ns+=i;
//	if (ne != -1) ne+=i;
//
//	SetLegatoTags(ns, dir, ne);
//
//    if (ne > 0)
//        ret = ne;
//    else if (ns > 0)
//        ret = ns;
//    return ret;
//}
//
//// -1 means not found
//void GetLegatoTags(int* start_out, int* end_out, int* interval_out, int* dir_out, bool shorthand) {
//	int note_start = -1, note_end = -1, interval = -1, d;
//	String dir;
//
//	String note_tag = tagmap["n"];
//
//	evaluate_legato_str(note_tag, note_start, note_end);
//
//	if (tagExists("i")) interval = (ToN)tagmap["i"];
//	if (tagExists("ns")) note_start = Midi::ToNum(tagmap["ns"]);
//	if (tagExists("ne")) note_end = Midi::ToNum(tagmap["ne"]);
//	if (tagExists("d")) dir = evaluate_dir_str(tagmap["d"]);
//
//	if (interval == -1 && note_start == -1 && note_end == -1 && dir == "") return;
//
//	if (note_start != -1 && note_end != -1) {
//		interval = abs(note_start - note_end);
//		dir = note_start > note_end ? "d" : note_start < note_end ? "u" : "r";
//	}
//	else if (interval !=-1 && dir != "r" && note_start != -1) {
//		note_end = dir == "d" ? note_start - interval : note_start + interval;
//	}
//	else if (interval !=-1 && dir != "r" && note_end != -1) {
//		note_start = dir == "u" ? note_end - interval : note_end + interval;
//	}
//
//	d = dir == "u" ? 1 : dir == "d" ? -1 : 0;
//
//	SetLegatoTags(note_start, d, note_end, shorthand);
//
//	if (start_out) *start_out = note_start;
//	if (end_out) *end_out = note_end;
//	if (interval_out) *interval_out = interval;
//	if (dir_out) *dir_out = d;
//}
//
//void SetLegatoTags(int ns, int dir, int ne, bool shorthand) {
//	String ns_str = Midi::ToName(ns);
//	String ne_str = Midi::ToName(ne);
//	String dir_str = dir  == 1 ? "up" : dir == -1 ? "dn" : "rp";
//	String d_str = dir == 1 ? "u" : dir == -1 ? "d" : "r";
//	String int_str = ne != -1 ? ToS(abs(ns-ne)) : "";
//
//	if ( int_str.size() && (!shorthand || tagExists("ns") || tagExists("ne")) ) {
//		removeTag("n");
//		SetTag("ns", ns_str);
//		SetTag("ne", ne_str);
//		SetTag("i", int_str);
//		SetTag("d", dir_str);
//	}
//	else if (tagExists("n") || tagExists("ns")) {
//		removeTag("ns");
//		if (ne != -1) SetTag("n", ns_str + d_str + ne_str);
//		else SetTag("n", ns_str);
//	}
//}
