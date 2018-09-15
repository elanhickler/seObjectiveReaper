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
  TagManager.setStringWithTags(getName()); 
}
ITEM::ITEM(MediaItem_Take * take) 
{
  item = GetMediaItemTake_Item(take); 
  TagManager.setStringWithTags(getName()); 
}

bool ITEM::IsGrouped(const ITEM & i1, const ITEM & i2, bool must_be_on_same_track)
{
    if (must_be_on_same_track && i1.getTrack() != i2.getTrack()) return false;
    return i1.getGroupIndex() && i1.getGroupIndex() == i2.getGroupIndex();
}

ITEM ITEM::Get(int idx) { return GetMediaItem(0, idx); }
ITEM ITEM::GetSelected(int idx) { return GetSelectedMediaItem(0, idx); }


ITEM ITEM::CreateMidi(MediaTrack * track, double position, double length)
{
  ITEM item = CreateNewMIDIItemInProj(track, position, position + length, 0);
  item.getActiveTake()->initMidi();
  return item;
}

String ITEM::getObjectName() const { return getActiveTake()->getName(); }
void ITEM::setObjectName(const String & v) 
{ 
  getActiveTake()->setName(v);
}
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

int ITEM::getIndex() const { return GetMediaItemInfo_Value(item, "IP_ITEMNUMBER"); }
MediaTrack * ITEM::getTrack() const { return GetMediaItem_Track(item); }
int ITEM::getTrackIndex() const { return GetMediaTrackInfo_Value(GetMediaItem_Track(item), "IP_TRACKNUMBER"); }
double ITEM::getSnapOffset() const { return GetMediaItemInfo_Value(item, "D_SNAPOFFSET"); }
bool ITEM::isMuted() const { return 0.0 != GetMediaItemInfo_Value(item, "B_MUTE"); }
int ITEM::getGroupIndex() const { return GetMediaItemInfo_Value(item, "I_GROUPID"); }
double ITEM::getVolume() const { return GetMediaItemInfo_Value(item, "D_VOL"); }
double ITEM::getFadeInLen() const { return GetMediaItemInfo_Value(item, "D_FADEINLEN"); }
double ITEM::getFadeOutLen() const { return GetMediaItemInfo_Value(item, "D_FADEOUTLEN"); }
double ITEM::getFadeInLenAuto() const { return GetMediaItemInfo_Value(item, "D_FADEINLEN_AUTO"); }
double ITEM::getFadeOutLenAuto() const { return GetMediaItemInfo_Value(item, "D_FADEOUTLEN_AUTO"); }
int ITEM::getFadeInShape() const { return GetMediaItemInfo_Value(item, "C_FADEINSHAPE"); }
int ITEM::getFadeOutShape() const { return GetMediaItemInfo_Value(item, "C_FADEOUTSHAPE"); }
double ITEM::getFadeInCurve() const { return GetMediaItemInfo_Value(item, "D_FADEINDIR"); }
double ITEM::getFadeOutCurve() const { return GetMediaItemInfo_Value(item, "D_FADEOUTDIR"); }
bool ITEM::isSelected() const { return 0.0 != GetMediaItemInfo_Value(item, "B_UISEL"); }
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

double ITEM::getRate() const { return getActiveTake()->getRate(); }

ITEM ITEM::duplicate()
{
		char* chunk = GetSetObjectState((MediaItem*)item, "");

    ITEM copy = SplitMediaItem(item, getStartPosition() + getLength()/2);

    GetSetObjectState(copy.item, chunk);
		GetSetObjectState(item, chunk);
    FreeHeapPtr(chunk);

    UpdateArrange();

    return copy;
}

void ITEM::setTrackByIndex(int v)
{
    int tracks = CountTracks(0);
    for (int t = tracks; t < v+1; ++t) InsertTrackAtIndex(t, true);
    MoveMediaItemToTrack(item, GetTrack(0, v));
}

bool ITEM::setTrack(MediaTrack * track) { 
    bool t = MoveMediaItemToTrack(item, track);
    UpdateArrange();
    return t;
}
bool ITEM::setTrackByName(String name) { return MoveMediaItemToTrack(item, TRACK::getByName(name)); }
void ITEM::setActiveTake(int idx) { SetActiveTake(GetTake(item, idx)); }
void ITEM::setSnapOffset(double v) { SetMediaItemInfo_Value(item, "D_SNAPOFFSET", v); }
void ITEM::setMuted(bool v) { SetMediaItemInfo_Value(item, "B_MUTE", v); }
void ITEM::setVolume(double v) { SetMediaItemInfo_Value(item, "D_VOL", v); }
void ITEM::move(double v) { SetMediaItemInfo_Value(item, "D_POSITION", GetMediaItemInfo_Value(item, "D_POSITION") + v); }

bool ITEM::crop(RANGE r, bool move_edge)
{
    bool start_was_moved = false, end_was_moved = false;
    if (getStartPosition() < r.start())
    {
        setStartPosition(r.start());
        start_was_moved = true;
    }
    if (getEndPosition() > r.end())
    {
        setEndPosition(r.end());
        end_was_moved = true;
    }
    return start_was_moved || end_was_moved;
}

void ITEM::setFadeInLen(double v) { SetMediaItemInfo_Value(item, "D_FADEINLEN", v); }
void ITEM::setFadeOutLen(double v) { SetMediaItemInfo_Value(item, "D_FADEOUTLEN", v); }
void ITEM::setFadeInLenAuto(double v) { SetMediaItemInfo_Value(item, "D_FADEINLEN_AUTO", v); }
void ITEM::setFadeOutLenAuto(double v) { SetMediaItemInfo_Value(item, "D_FADEOUTLEN_AUTO", v); }
void ITEM::setFadeInShape(int v) { SetMediaItemInfo_Value(item, "C_FADEINSHAPE", v); }
void ITEM::setFadeOutShape(int v) { SetMediaItemInfo_Value(item, "C_FADEOUTSHAPE", v); }
void ITEM::setFadeInCurve(double v) { SetMediaItemInfo_Value(item, "D_FADEINDIR", v); }
void ITEM::setFadeOutCurve(double v) { SetMediaItemInfo_Value(item, "D_FADEOUTDIR", v); }
void ITEM::setSelected(bool v) { SetMediaItemInfo_Value(item, "B_UISEL", v); }

void ITEM::setRate(double new_rate, bool warp)
{
    if (warp)
    {
        double old_rate = getRate();
        if (old_rate == new_rate) return;

        double ratio = old_rate / new_rate;

        // set length based on rate change
        setLength(getLength() * ratio);

        // set snap offset based on rate change
        setSnapOffset(getSnapOffset() * ratio);

        // adjust all takes' rates
        TAKELIST TakesList = GetTakes();
        for (auto& take : TakesList)
        {
            take.setPreservePitch(false);
            take.setRate(new_rate);
        }
    }
    else
        setRate(new_rate);
}

String ITEM::GetPropertyStringFromKey(const String & key, bool get_value) const
{
    auto iter = method_lookup.find(key);

    if (iter == method_lookup.end())
        return getActiveTake()->getTag(key);

    switch (iter->second)
    {
    case __name:
        return getActiveTake()->getStringNoTags();
    case __track:
        if (get_value) return (String)TRACK(getTrack()).idx();
        else return TRACK(getTrack()).getName();
    case __length:
        return (String)getLength();
    case __rate:
        return (String)getRate();
    case __volume:
        return (String)getVolume();
    case __snapoffset:
        return (String)getSnapOffset();
    case __position:
        return (String)getStartPosition();
    case __fadeinlen:
        return (String)getFadeInLen();
    case __fadeoutlen:
        return (String)getFadeOutLen();
    case __startoffset:
        return (String)getActiveTake()->getStartOffset();
    case __tags:
        return getActiveTake()->getNameTagsOnly();
    case __pitch:
        return (String)getActiveTake()->getPitch();
    case __file_extension:
        return getActiveTake()->file().getFileExtension();
    }

    return String();
}

int ITEMLIST::CountSelected() { return CountSelectedMediaItems(0); }

int ITEMLIST::Count() { return CountMediaItems(0); }

void ITEMLIST::CollectItems()
{
  int items = CountMediaItems(0);

  if (items == 0)
    return;

  list.reserve(items);

  for (int i = 0; i < items; ++i)
    push_back(GetMediaItem(0, i));

  if (do_sort)
  {
    sort();
    r = RANGE({ front().getStartPosition(), list.back().getEndPosition() });
  }
}

void ITEMLIST::CollectSelectedItems()
{
    int items = CountSelectedMediaItems(0);

    if (items == 0) 
      return;

    list.reserve(items);

    for (int i = 0; i < items; ++i)
        push_back(GetSelectedMediaItem(0, i));

    if (do_sort)
    {
        sort();
        r = RANGE({ front().getStartPosition(), list.back().getEndPosition() });
    }
}

double ITEMLIST::getStartPosition() const { return front().getStartPosition(); }
double ITEMLIST::getEndPosition() const { return back().getEndPosition(); }
double ITEMLIST::getLength() const { return back().getEndPosition() - front().getStartPosition(); }
double ITEMLIST::getSnapOffset() const { return front().getSnapOffset(); }
double ITEMLIST::getFadeInLen() const { return front().getFadeInLen(); }
double ITEMLIST::getFadeOutLen() const { return back().getFadeOutLen(); }
String ITEMLIST::GetPropertyStringFromKey(const String & key, bool use_value) const
{
    return front().GetPropertyStringFromKey(key, use_value);
}

bool ITEMLIST::isSelected() const
{
    for (const auto & i : list)
        if (!i.isSelected())
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

int ITEMLIST::setTrack(MediaTrack * track)
{
    int num_items_moved = 0;
    for (auto& item : list)
        if (MoveMediaItemToTrack(item, track)) 
            ++num_items_moved;

    return num_items_moved;
}

void ITEMLIST::setStartPosition(double v)
{
  double posDiff = v - list[0].getPosition();
  for (ITEM & item : list)
    item.move(posDiff);
}

void ITEMLIST::setEndPosition(double v)
{
    list.back().setEndPosition(v);
}

void ITEMLIST::setSnapOffset(double v)
{
    int i = 0;
    while (i < list.size() && !RANGE::is_touching(list[i], v))
        ++i;
    if (i < list.size())
        list[i].setSnapOffset(v);
    else if (v < front().getStartPosition())
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

void ITEMLIST::setSelected(bool select) 
{ 
    for (auto& item : list) 
        item.setSelected(select); 
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
    t.collectItems();

    if (selected_only)
    {
      if (t.getSelectedItemList().empty())
        continue;
    }
    else
    {
      if (t.empty())
        continue;
    }

    // get either selected list or normal list
    ITEMLIST * il = selected_only ? &t.ItemList_selected : &t.getItemList();
    ITEM previous_item = selected_only ? t.getSelectedItemList().front() : t.front();
    
    addNewList()->push_back(previous_item);

    for (const ITEM & i : *il)
    {
      if (previous_item == i)
        continue;

      bool do_new_list = must_be_overlapping ? !RANGE::is_overlapping(previous_item, i) : !RANGE::is_touching(previous_item, i);

      if (do_new_list)
        addNewList()->push_back(i); 
      else 
        back().push_back(i);

      previous_item = i;
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

    if (do_sort) 
      sort();
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