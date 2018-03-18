#include "ReaperClassesHeader.h"
#include "Item.h"

ITEM::ITEM() 
{ 
  item = nullptr; 
}
ITEM::ITEM(MediaItem * item) : item(item) 
{ 
  jassert(item != nullptr);
  active_take = GetActiveTake(item); 
  TagManager.WithTags(name()); 
}
ITEM::ITEM(MediaItem_Take * take) 
{
  item = GetMediaItemTake_Item(take); 
  TagManager.WithTags(name()); 
}

bool ITEM::is_grouped(const ITEM & i1, const ITEM & i2, bool must_be_on_same_track)
{
    if (must_be_on_same_track && i1.track() != i2.track()) return false;
    return i1.getGroupIndex() && i1.getGroupIndex() == i2.getGroupIndex();
}

ITEM ITEM::get(int idx) { return GetMediaItem(0, idx); }
ITEM ITEM::getSelected(int idx) { return GetSelectedMediaItem(0, idx); }
String ITEM::getObjectName() const { return getActiveTake()->name(); }
void ITEM::setObjectName(const String & v) { getActiveTake()->name(v); }
double ITEM::getObjectStartPos() const { return GetMediaItemInfo_Value(item, "D_POSITION"); }

void ITEM::setObjectStartPos(double v)
{
    UNSELECT_ITEMS();
    SetMediaItemInfo_Value(item, "B_UISEL", true);
    NUDGE::START(v, false);
}

double ITEM::getObjectLength() const { return GetMediaItemInfo_Value(item, "D_LENGTH"); }
void ITEM::setObjectLength(double v) { SetMediaItemInfo_Value(item, "D_LENGTH", v); }
void ITEM::setObjectPosition(double v) { SetMediaItemInfo_Value(item, "D_POSITION", v); }
int ITEM::getObjectColor() const { return 0; }
void ITEM::setObjectColor(int v) {}
bool ITEM::objectIsValid() const { return item != nullptr; }

void ITEM::remove() { DeleteTrackMediaItem(GetMediaItemTrack(item), item); item = nullptr; }
ITEM ITEM::split(double v) 
{
  MediaItem * i = SplitMediaItem(item, v);
  if (i == nullptr)
    return item;

  return i;
}

ITEMLIST ITEM::split(vector<double> splitlist)
{
  ITEMLIST SplitItems(item);

  stable_sort(splitlist.begin(), splitlist.end(), [](double a, double b) { return a < b; });

  for (const auto & v : splitlist)
    SplitItems.push_back(SplitItems.back().split(v));

  return SplitItems;
}

TAKELIST ITEM::GetTakes()
{
    return TakeList;
}
void ITEM::CollectTakes()
{
    TakeList.clear();
    for (int t = 0; t < CountTakes(item); ++t) 
        TakeList.push_back(GetTake(item, t));
    active_take = GetActiveTake(item);
}

int ITEM::idx() const { return GetMediaItemInfo_Value(item, "IP_ITEMNUMBER"); }
MediaTrack * ITEM::track() const { return GetMediaItem_Track(item); }
int ITEM::track_idx() const { return GetMediaTrackInfo_Value(GetMediaItem_Track(item), "IP_TRACKNUMBER"); }
double ITEM::getSnapOffset() const { return GetMediaItemInfo_Value(item, "D_SNAPOFFSET"); }
bool ITEM::getIsMuted() const { return 0.0 != GetMediaItemInfo_Value(item, "B_MUTE"); }
int ITEM::getGroupIndex() const { return GetMediaItemInfo_Value(item, "I_GROUPID"); }
double ITEM::vol() const { return GetMediaItemInfo_Value(item, "D_VOL"); }
double ITEM::fadeinlen() const { return GetMediaItemInfo_Value(item, "D_FADEINLEN"); }
double ITEM::fadeoutlen() const { return GetMediaItemInfo_Value(item, "D_FADEOUTLEN"); }
double ITEM::fadeinlen_auto() const { return GetMediaItemInfo_Value(item, "D_FADEINLEN_AUTO"); }
double ITEM::fadeoutlen_auto() const { return GetMediaItemInfo_Value(item, "D_FADEOUTLEN_AUTO"); }
int ITEM::setFadeInShape() const { return GetMediaItemInfo_Value(item, "C_FADEINSHAPE"); }
int ITEM::setFadeOutShape() const { return GetMediaItemInfo_Value(item, "C_FADEOUTSHAPE"); }
double ITEM::fadein_curve() const { return GetMediaItemInfo_Value(item, "D_FADEINDIR"); }
double ITEM::fadeout_curve() const { return GetMediaItemInfo_Value(item, "D_FADEOUTDIR"); }
bool ITEM::getIsSelected() const { return 0.0 != GetMediaItemInfo_Value(item, "B_UISEL"); }
const TAKE * ITEM::getActiveTake() const { return &active_take; }
const TAKE * ITEM::getTake(int i) const
{
    if (i >= TakeList.size())
        return &InvalidTake;
    return &TakeList[i];
}

TAKE * ITEM::getActiveTake() { return &active_take;  }

TAKE * ITEM::getTake(int i) {
    if (i >= TakeList.size())
        return &InvalidTake;
    return &TakeList[i];
}

int ITEM::getNumTakes() { return CountTakes(item); }

double ITEM::getRate() const { return getActiveTake()->rate(); }

ITEM ITEM::duplicate()
{
		char* chunk = GetSetObjectState(item, "");

    ITEM copy = SplitMediaItem(item, startPos() + length()/2);

    GetSetObjectState(copy.item, chunk);
		GetSetObjectState(item, chunk);
    FreeHeapPtr(chunk);

    UpdateArrange();

    return copy;
}

void ITEM::track_idx(int v)
{
    int tracks = CountTracks(0);
    for (int t = tracks; t < v+1; ++t) InsertTrackAtIndex(t, true);
    MoveMediaItemToTrack(item, GetTrack(0, v));
}

void ITEM::track(int v) { track_idx(v); }
bool ITEM::track(MediaTrack * track) { 
    bool t = MoveMediaItemToTrack(item, track);
    UpdateArrange();
    return t;
}
bool ITEM::track(String name) { return MoveMediaItemToTrack(item, TRACK::get(name)); }
void ITEM::activeTake(int idx) { SetActiveTake(GetTake(item, idx)); }
void ITEM::setSnapOffset(double v) { SetMediaItemInfo_Value(item, "D_SNAPOFFSET", v); }
void ITEM::setIsMuted(bool v) { SetMediaItemInfo_Value(item, "B_MUTE", v); }
void ITEM::vol(double v) { SetMediaItemInfo_Value(item, "D_VOL", v); }
void ITEM::move(double v) { SetMediaItemInfo_Value(item, "D_POSITION", GetMediaItemInfo_Value(item, "D_POSITION") + v); }

bool ITEM::crop(RANGE r, bool move_edge)
{
    bool start_was_moved = false, end_was_moved = false;
    if (startPos() < r.start())
    {
        startPos(r.start());
        start_was_moved = true;
    }
    if (endPos() > r.end())
    {
        endPos(r.end());
        end_was_moved = true;
    }
    return start_was_moved || end_was_moved;
}

void ITEM::fadeinlen(double v) { SetMediaItemInfo_Value(item, "D_FADEINLEN", v); }
void ITEM::fadeoutlen(double v) { SetMediaItemInfo_Value(item, "D_FADEOUTLEN", v); }
void ITEM::fadeinlen_auto(double v) { SetMediaItemInfo_Value(item, "D_FADEINLEN_AUTO", v); }
void ITEM::fadeoutlen_auto(double v) { SetMediaItemInfo_Value(item, "D_FADEOUTLEN_AUTO", v); }
void ITEM::fadein_shape(int v) { SetMediaItemInfo_Value(item, "C_FADEINSHAPE", v); }
void ITEM::fadeout_shape(int v) { SetMediaItemInfo_Value(item, "C_FADEOUTSHAPE", v); }
void ITEM::fadein_curve(double v) { SetMediaItemInfo_Value(item, "D_FADEINDIR", v); }
void ITEM::fadeout_curve(double v) { SetMediaItemInfo_Value(item, "D_FADEOUTDIR", v); }
void ITEM::setIsSelected(bool v) { SetMediaItemInfo_Value(item, "B_UISEL", v); }

void ITEM::rate(double new_rate, bool warp)
{
    if (warp)
    {
        double old_rate = getRate();
        if (old_rate == new_rate) return;

        double ratio = old_rate / new_rate;

        // set length based on rate change
        length(length() * ratio);

        // set snap offset based on rate change
        setSnapOffset(getSnapOffset() * ratio);

        // adjust all takes' rates
        TAKELIST TakesList = GetTakes();
        for (auto& take : TakesList)
        {
            take.preservepitch(false);
            take.rate(new_rate);
        }
    }
    else
        rate(new_rate);
}

String ITEM::GetPropertyStringFromKey(const String & key, bool get_value) const
{
    auto iter = method_lookup.find(key);

    if (iter == method_lookup.end())
        return getActiveTake()->getTag(key);

    switch (iter->second)
    {
    case __name:
        return getActiveTake()->nameNoTags();
    case __track:
        if (get_value) return (String)TRACK(track()).idx();
        else return TRACK(track()).name();
    case __length:
        return (String)length();
    case __rate:
        return (String)getRate();
    case __volume:
        return (String)vol();
    case __snapoffset:
        return (String)getSnapOffset();
    case __position:
        return (String)startPos();
    case __fadeinlen:
        return (String)fadeinlen();
    case __fadeoutlen:
        return (String)fadeoutlen();
    case __startoffset:
        return (String)getActiveTake()->offset();
    case __tags:
        return getActiveTake()->nameTagsOnly();
    case __pitch:
        return (String)getActiveTake()->pitch();
    case __file_extension:
        return getActiveTake()->file().getFileExtension();
    }

    return String();
}

void ITEMLIST::CollectItems()
{
    int items = CountMediaItems(0);
    for (int i = 0; i < items; ++i)
        ITEM item = GetMediaItem(0, i);
    if (do_sort)
    {
        sort();
        r = RANGE({ front().startPos(), list.back().endPos() });
    }
}

void ITEMLIST::CollectSelectedItems()
{
    int items = CountSelectedMediaItems(0);

    if (items == 0) return;

    list.reserve(items);
    for (int i = 0; i < items; ++i)
        push_back(GetSelectedMediaItem(0, i));
    if (do_sort)
    {
        sort();
        r = RANGE({ front().startPos(), list.back().endPos() });
    }
}

double ITEMLIST::startPos() const { return front().startPos(); }
double ITEMLIST::endPos() const { return back().endPos(); }
double ITEMLIST::length() const { return back().endPos() - front().startPos(); }
double ITEMLIST::snap() const { return front().getSnapOffset(); }
double ITEMLIST::fadeinlen() const { return front().fadeinlen(); }
double ITEMLIST::fadeoutlen() const { return back().fadeoutlen(); }
String ITEMLIST::GetPropertyStringFromKey(const String & key, bool use_value) const
{
    return front().GetPropertyStringFromKey(key, use_value);
}

bool ITEMLIST::selected() const
{
    for (const auto & i : list)
        if (!i.getIsSelected())
            return false;

    return true;
}

int ITEMLIST::crop(RANGE rng, bool move_edge)
{
    int num_cropped = 0;
    for (auto & i : list)
        if (i.crop(rng, move_edge))
            ++num_cropped;

    return num_cropped;
}

int ITEMLIST::track(MediaTrack * track)
{
    int num_items_moved = 0;
    for (auto& item : list)
        if (MoveMediaItemToTrack(item, track)) 
            ++num_items_moved;

    return num_items_moved;
}

int ITEMLIST::track(String name) { return TRACK::get(name).idx(); }

void ITEMLIST::endPos(double v)
{
    list.back().endPos(v);
}

void ITEMLIST::setSnapOffset(double v)
{
    int i = 0;
    while (i < list.size() && !RANGE::is_touching(list[i], v))
        ++i;
    if (i < list.size())
        list[i].setSnapOffset(v);
    else if (v < front().startPos())
        front().setSnapOffset(v);
    else
        list.back().setSnapOffset(v);
}

void ITEMLIST::move(double v)
{
    for (auto & i : list)
        i.move(v);
}

void ITEMLIST::remove()
{
    for (auto& item : list)
        item.remove();
}

void ITEMLIST::selected(bool select) 
{ 
    for (auto& item : list) 
        item.setIsSelected(select); 
}

// functions

void ITEMLIST::InitAudio()
{
    for (auto & item : list) 
      item.getActiveTake()->initAudio();
}

void ITEMGROUPLIST::collect_donotgroup(bool selected_only)
{
    ITEMLIST il;
    if (selected_only) 
        il.CollectSelectedItems();
    else
        il.CollectItems();

    push_back(il);

    reserve(il.size());
}

void ITEMGROUPLIST::collect_groupgrouped(bool selected_only)
{
    int items = selected_only ? CountSelectedMediaItems(0) : CountMediaItems(0);
    reserve(items);

    ITEMLIST il;
    if (selected_only)
        il.CollectSelectedItems();
    else 
        il.CollectItems();

    map<int, ITEMLIST> group_map;
    set<int> group_set;

    int non_group_counter = 0;

    for (ITEM & i : il)
    {        
        int grp = i.getGroupIndex();
        if (grp == 0) grp = non_group_counter--;
        group_map[grp].push_back(i);
        group_set.insert(grp);
    }

    for (auto it = group_set.begin(); it != group_set.end(); ++it)
        push_back(group_map[*it]);
}

void ITEMGROUPLIST::collect_groupoverlapping(bool selected_only, bool must_be_overlapping)
{
    TRACKLIST tl;
    tl.CollectTracks();

    // go through each track
    for (TRACK & t : tl)
    {
        t.CollectItems();
        ITEM last_item = t[0];

        // go through each item in track
        push_back(ITEMLIST());
        back().reserve(t.getSelectedItemList()->size());

        // get either selected list or normal list
        ITEMLIST * il;
        if (selected_only)
            il = t.getSelectedItemList();
        else
            il = dynamic_cast<ITEMLIST *>(t.getItemList());

        if (must_be_overlapping)
            for (const ITEM & i : *il)
            {
                // if last item overlaps current item, add to last group
                if (RANGE::is_overlapping(last_item, i))
                    back().push_back(i);
                // otherwise create a new item group with this item
                else
                    push_back(ITEMLIST(i));
                last_item = i;
            }
        else
            for (const ITEM & i : *il)
            {
                // if last item touching current item, add to last group
                if (RANGE::is_touching(last_item, i))
                    back().push_back(i);
                // otherwise create a new item group with this item
                else
                    push_back(ITEMLIST(i));
                last_item = i;
            }
    }
}

void ITEMGROUPLIST::CollectSelectedItems(int group_mode)
{
    switch (group_mode)
    {
    case 0: // do not group items
        collect_donotgroup(true);
        break;

    case 1: // group grouped items
        collect_groupgrouped(true);
        break;

    case 2: // group overlapping items
        collect_groupoverlapping(true, true);
        break;

    case 3: // group touching items
        collect_groupoverlapping(true, false);
        break;
    }

    if (do_sort) sort();
}

void ITEMGROUPLIST::CollectItems(int group_mode)
{
    switch (group_mode)
    {
    case 0: // do not group items
        collect_donotgroup(false);
        break;

    case 1: // group grouped items
        collect_groupgrouped(false);
        break;

    case 2: // group overlapping items
        collect_groupoverlapping(false, true);
        break;

    case 3: // group touching items
        collect_groupoverlapping(false, false);
        break;
    }

    if (do_sort) sort();
}

int ITEMGROUPLIST::countItems()
{
    int c = 0;
    for (const auto & itemgroup : list)
        c += itemgroup.size();
    return c;
}