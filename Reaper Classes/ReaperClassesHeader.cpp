#include "../reaper plugin/reaper_plugin_functions.h"

#include "ReaperClassesHeader.h"

#undef min
#undef max

extern reaper_plugin_info_t* g_plugin_info;

action_entry::action_entry(std::string description, std::string idstring, toggle_state togst, std::function<void(action_entry&)> func) :
	m_desc(description), m_id_string(idstring), m_func(func), m_togglestate(togst)
{
	if (g_plugin_info != nullptr)
	{
		m_accel_reg.accel = { 0,0,0 };
		m_accel_reg.desc = m_desc.c_str();
		m_accel_reg.accel.cmd = m_command_id = g_plugin_info->Register("command_id", (void*)m_id_string.c_str());
		g_plugin_info->Register("gaccel", &m_accel_reg);
	}
}

void msg(String s) { ShowConsoleMsg(s.toRawUTF8()); }

void COMMAND(int action, int flag) { Main_OnCommand(action, flag); }
void COMMAND(const char* action, int flag) { Main_OnCommand(NamedCommandLookup(action), flag); }

double SETCURSOR(double time, bool moveview, bool seekplay) { SetEditCurPos(time, moveview, seekplay); return time; }
double GETCURSOR() { return GetCursorPosition(); }

double saved_cursor_position;
void SAVE_CURSOR() { saved_cursor_position = GETCURSOR(); }
void RESTORE_CURSOR() { SETCURSOR(saved_cursor_position); }

void UNSELECT_ITEMS() { COMMAND(40289); }
MediaItem* SELECT_ITEM_UNDER_MOUSE() { Main_OnCommand(40528, 0); return GetSelectedMediaItem(0, 0); }

int juceToReaperColor(const Colour & v)
{
	return ColorToNative(v.getRed(), v.getGreen(), v.getBlue()) | 0x01000000;
}

Colour reaperToJuceColor(int v)
{
	int r, g, b;
	ColorFromNative(v, &r, &g, &b);
	return Colour(r, g, b);
}

void SplitTakeChunks(MediaItem * item, string chunk_c, string & header, string & footer, vector<string>& take_chunks, int & act_take_num)
{
	string chunk = chunk_c;

	// Split item chunk
	match_results<string::const_iterator> m;
	regex_search(chunk, m, chunk_to_sections);
	header = m[1];
	string data = m[2];
	footer = m[3];

	// Split data into take chunks
	sregex_token_iterator i(data.begin(), data.end(), separate_take_chunks, { -1, 0 });
	sregex_token_iterator j;
	act_take_num = 0;
	++i;
	while (i != j)
	{
		take_chunks.push_back(*i++);
		if (i != j) take_chunks.back() += *i++;
		if (!act_take_num) act_take_num = take_chunks.back().substr(0, 8) == "TAKE SEL" ? take_chunks.size() - 1 : 0;
	}
}

// envelope functions
TrackEnvelope * ToggleTakeEnvelopeByName(MediaItem_Take * take, string env_name, bool off_on)
{
	bool update_chunk = false;
	bool env_is_enabled = false;
	string header, footer, chunk;
	vector<string> take_chunks;
	int act_take_num;
	int take_num = GetMediaItemTakeInfo_Value(take, "IP_TAKENUMBER");
	auto item = (MediaItem*)GetSetMediaItemTakeInfo(take, "P_ITEM", 0);

	char* chunk_c = GetSetObjectState(item, "");
	SplitTakeChunks(item, chunk_c, header, footer, take_chunks, act_take_num);

	auto idx = TakeEnvMap[env_name];

	if (regex_search(take_chunks[take_num], idx.r_search)) env_is_enabled = true;

	if (off_on && !env_is_enabled)
	{
		take_chunks[take_num] += idx.defchunk;
		update_chunk = true;
	}
	else if (!off_on && env_is_enabled)
	{
		take_chunks[take_num] = regex_replace(take_chunks[take_num], idx.r_replace, "$1");
		update_chunk = true;
	}

	// Rebuild item chunk
	if (update_chunk)
	{
		chunk = header;
		for (const auto& c : take_chunks) chunk += c;
		chunk += footer;

		GetSetObjectState(item, chunk.c_str());

		FreeHeapPtr(chunk_c);
	}

	return off_on ? GetTakeEnvelopeByName(take, env_name.c_str()) : nullptr;
}

double PROJECT::getEndTime()
{
	SAVE_CURSOR();
	VIEW();

	COMMAND(40043); // Transport: Go to end of project
	double time = GETCURSOR();

	RESTORE_CURSOR();
	VIEW();

	return time;
}

double PROJECT::getGridDivision()
{
	double division;
	GetSetProjectGrid(nullptr, false, &division, nullptr, nullptr);
	return division;
}

double PROJECT::getGridDivisionTime()
{
	return 120.0 / PROJECT::getTempo() * 2.0 * PROJECT::getGridDivision();
}

double PROJECT::getTempo()
{
	return Master_GetTempo();
}

String PROJECT::getFilePath()
{
	char charbuf[1024];
	EnumProjects(-1, charbuf, 1024);
	return charbuf;
}

String PROJECT::getDirectory()
{
	return File(PROJECT::getFilePath()).getParentDirectory().getFullPathName();
}

int PROJECT::countMakersAndRegions()
{
	int m, r;
	CountProjectMarkers(nullptr, &m, &r);
	return m + r;
}

bool ui_is_updating = true;
void UI()
{
	if (ui_is_updating) { PreventUIRefresh(1); ui_is_updating = false; }
	else { PreventUIRefresh(-1); ui_is_updating = true; UPDATE(); }
}

bool undo_is_active = false;
void UNDO(String undostr, ReaProject* project)
{
	if (!undo_is_active) Undo_BeginBlock2(0);
	else Undo_EndBlock2(project, undostr.toRawUTF8(), -1);

	flip(undo_is_active);
}

bool view_is_being_saved = true;
double global_save_view_start = 0;
double global_save_view_end = 0;
void VIEW()
{
	if (view_is_being_saved)
	{
		GetSet_ArrangeView2(0, 0, 0, 0, &global_save_view_start, &global_save_view_end);
		view_is_being_saved = false;
	}
	else
	{
		GetSet_ArrangeView2(0, 1, 0, 0, &global_save_view_start, &global_save_view_end);
		view_is_being_saved = true;
	}
}

void UPDATE()
{
	UpdateArrange();
	TrackList_AdjustWindows(false);
}

void SET_ALL_ITEMS_OFFLINE() { COMMAND(40100); }
void SET_ALL_ITEMS_ONLINE() { COMMAND(40101); }
void SET_SELECTED_ITEMS_OFFLINE() { COMMAND(40440); }
void SET_SELECTED_ITEMS_ONLINE() { COMMAND(40439); }
void PASTE_ITEMS() { COMMAND(40058); }
void REMOVE_ITEMS() { COMMAND(40006); }
void GLUE_ITEMS() { COMMAND(40362); }

void NUDGE::START(double v, bool move_source)
{
	int nudgewhat = 1;
	if (move_source) nudgewhat = 2;
	ApplyNudge(0, 0, nudgewhat, 1, v, 0, 0);
}

//double GetNextItemStartTime(double time, TRACKLIST & TrackListIn)
//{
//    return 0.0;
//}
//
//double GetNearestItemStartTime(double time, TRACKLIST & TrackListIn)
//{
//    ITEMLIST il;
//
//    for (auto& track : TrackListIn)
//    {
//        if (track.selectedItemList().empty()) continue;
//        il.push_back(GetNearestItemToTimeBasedOnStart(time, track.selectedItemList()));
//    }
//
//    return GetNearestItemToTimeBasedOnStart(time, il).start();
//}
//
//double GetNearestItemEndTime(double time, TRACKLIST & TrackListIn)
//{
//    ITEMLIST il;
//
//    for (auto& track : TrackListIn)
//    {
//        if (track.selectedItemList().empty()) continue;
//        il.push_back(GetNearestItemToTimeBasedOnEnd(time, track.selectedItemList()));
//    }
//
//    return GetNearestItemToTimeBasedOnEnd(time, il).end();
//}
//
//ITEM GetNearestItemToTimeBasedOnStart(double time, ITEMLIST & ItemListIn)
//{
//    if (ItemListIn.size() == 1)
//        return ItemListIn.back();
//
//    int i = 0;
//    for (; i < ItemListIn.size(); ++i)
//    {
//        if (ItemListIn[i].start() > time)
//            break;
//    }
//
//    if (i == 0) 
//        return ItemListIn[0];
//
//    if (i == ItemListIn.size()) 
//        return ItemListIn.back();
//
//    ITEM & Item1 = ItemListIn[i];
//    ITEM & Item2 = ItemListIn[i-1];
//
//    if (abs(Item1.start() - time) < abs(Item2.start() - time))
//        return Item1;
//    else
//        return Item2;
//}
//
//ITEM GetNextItemToTimeBasedOnStart(double time, ITEMLIST & ItemList, int starting_idx)
//{
//    int i = starting_idx;
//    ITEM item;    
//    for (; i < ItemList.size(); ++i)
//    {
//        item = ItemList[i];
//        if (item.start() > time) break;
//    }
//    return item;
//}
//
//ITEM GetNearestItemToTimeBasedOnEnd(double time, ITEMLIST & ItemListIn)
//{
//    if (ItemListIn.size() == 1)
//        return ItemListIn.back();
//
//    int i = 0;
//    for (; i < ItemListIn.size(); ++i)
//    {
//        if (ItemListIn[i].end() > time)
//            break;
//    }
//
//    if (i == 0)
//        return ItemListIn[0];
//
//    if (i == ItemListIn.size())
//        return ItemListIn.back();
//
//    ITEM & Item1 = ItemListIn[i];
//    ITEM & Item2 = ItemListIn[i-1];
//
//    if (abs(Item1.end() - time) < abs(Item2.end() - time))
//        return Item1;
//    else
//        return Item2;
//}
//
//RANGE GetRangeOfSelectedItems(const TRACKLIST & TrackListIn)
//{
//    if (TrackListIn.empty() || TrackListIn[0].empty())
//        return RANGE();
//
//    RANGE range = TrackListIn[0][0].start();
//
//    for (const auto& Track : TrackListIn)
//        for (const auto& item : Track.selectedItemsList())
//            range.expand(item.start(), item.end());
//
//    return range;
//}
//
//ITEMGROUPLIST GetGroupsOfItems(const vector<RANGE> & list, TRACKLIST & TrackListIn)
//{
//    ITEMGROUPLIST ItemGroupList;
//
//    // go through each trange
//    for (const auto& range : list)
//    {
//        ITEMLIST ItemList;
//
//        // go through each track
//        for (int track_idx = 0; track_idx < TrackListIn.size(); ++track_idx)
//        {
//            TRACK & track = TrackListIn[track_idx];
//
//            // skip track if no selected items
//            if (!track.has_selected_items()) continue;
//
//            // go to first item on track in range
//            int num_sel_items_on_track = track.countSelectedItems();
//            int item_idx;
//            for (item_idx = 0; item_idx < num_sel_items_on_track; ++item_idx)
//                if (track.getSelectedItem(item_idx).start() >= range.start())
//                    break;
//
//            // go through each item and append item until out of range
//            for (item_idx; item_idx < num_sel_items_on_track; ++item_idx)
//            {
//                auto item = track.getSelectedItem(item_idx);
//                if (item.start() < range.end())
//                    ItemList.push_back(item);
//                else
//                    break;
//            }
//        }
//
//        // after appending all qualifying items to list, append list to group list
//        ItemGroupList.push_back(ItemList);
//    }
//    return std::move(ItemGroupList);
//}
//
//MARKERLIST GetMarkersInRange(RANGE range, MARKERLIST & MarkerListIn,  bool createGhostMarkers)
//{
//    MARKERLIST ml;
//
//    if (MarkerListIn.empty())
//        return MARKERLIST();
//
//    // if first marker is not at range start, create ghost marker if wanted
//    if (createGhostMarkers && MarkerListIn[0].start() > range.start())
//        ml.push_back(MARKER::createGhost(range.start()));
//
//    // get markers of list inside range
//    for (int i = 0; i < MarkerListIn.size(); ++i)
//    {
//        MARKER & m = MarkerListIn[i];
//        if (m.start() >= range.start() && m.start() <= range.end())
//            ml.push_back(m);
//        if (m.start() > range.end())
//            break;
//    }
//    
//    if (createGhostMarkers)
//    {
//        // if no markers found in range, create ghost marker at range start
//        if (ml.empty())
//            ml.push_back(MARKER::createGhost(range.start()));
//
//        // if no marker found at range end, create ghost marker
//        if (ml.back().start() < range.end())
//            ml.push_back(MARKER::createGhost(range.end()));
//    }
//
//    return std::move(ml);
//}
//
//MARKER GetNextMarkerOrProjectEnd(double time, MARKERLIST & MarkerListIn, int starting_idx)
//{
//    int i = starting_idx;
//    MARKER & marker = MarkerListIn[i];
//    for (; i < MarkerListIn.size(); ++i)
//    {
//        marker = MarkerListIn[i];
//        if (marker.start() > time) break;
//    }
//    return marker;
//}

//String GetPropertyStringFromKey(const String & key, bool use_value) const { return String(); }
