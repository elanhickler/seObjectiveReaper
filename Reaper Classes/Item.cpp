#include "ReaperClassesHeader.h"
#include "Item.h"

ITEM::ITEM()
{
	itemPtr = nullptr;
}

void ITEM::collectTakes()
{
	int num_takes = CountTakes(itemPtr);
	for (int i = 0; i < num_takes; ++i)
	{
		push_back(GetTake(itemPtr, i));
		back().itemParent = this;
		back().initAudio();
	}
}

ITEM::ITEM(MediaItem * item) : itemPtr(item)
{
	jassert(item != nullptr);
	collectTakes();
	TagManager.setStringWithTags(getName());
}
ITEM::ITEM(MediaItem_Take * take)
{
	jassert(take != nullptr);
	itemPtr = GetMediaItemTake_Item(take);
	collectTakes();
	TagManager.setStringWithTags(getName());
}

const TAKE & ITEM::getActiveTake() const
{
	int idx = getActiveTakeIndex();
	jassert(size() > idx);
	return list[idx];
}

TAKE & ITEM::getActiveTake()
{
	int idx = getActiveTakeIndex();
	jassert(size() > idx);
	return list[idx];
}

const TAKE & ITEM::getTake(int i) const
{
	jassert(size() > i);
	return list[i];
}
TAKE & ITEM::getTake(int i)
{
	jassert(size() > i);
	return list[i];
}

bool ITEM::IsGrouped(const ITEM & i1, const ITEM & i2, bool must_be_on_same_track)
{
	if (must_be_on_same_track && i1.getTrack() != i2.getTrack()) return false;
	return i1.getGroupIndex() && i1.getGroupIndex() == i2.getGroupIndex();
}

ITEM ITEM::get(int idx) { return GetMediaItem(0, idx); }
ITEM ITEM::getSelected(int idx) { return GetSelectedMediaItem(0, idx); }

ITEM ITEM::createMidi(MediaTrack * track, double position, double length)
{
	ITEM item = CreateNewMIDIItemInProj(track, position, position + length, 0);
	return item;
}

String ITEM::getName() const { return getActiveTake().getName(); }
void ITEM::setName(const String & v)
{
	getActiveTake().setName(v);
}
double ITEM::getStart() const { return GetMediaItemInfo_Value(itemPtr, "D_POSITION"); }

void ITEM::setStart(double v)
{
	UNSELECT_ITEMS();
	SetMediaItemInfo_Value(itemPtr, "B_UISEL", true);
	NUDGE::START(v, false);
}

double ITEM::getEnd() const
{
	return getStart() + getLength();
}

void ITEM::setEnd(double v)
{
	setLength(v - getStart());
}

double ITEM::getLength() const { return GetMediaItemInfo_Value(itemPtr, "D_LENGTH"); }
void ITEM::setLength(double v) { SetMediaItemInfo_Value(itemPtr, "D_LENGTH", v); }
void ITEM::setPosition(double v) { SetMediaItemInfo_Value(itemPtr, "D_POSITION", v); }
Colour ITEM::getColor() const
{
	return reaperToJuceColor(GetMediaItemInfo_Value(itemPtr, "I_CUSTOMCOLOR"));
}
void ITEM::setColor(Colour v)
{
	SetMediaItemInfo_Value(itemPtr, "I_CUSTOMCOLOR", juceToReaperColor(v));
}

bool ITEM::isValid() const { return itemPtr != nullptr; }

void ITEM::remove() { DeleteTrackMediaItem(GetMediaItemTrack(itemPtr), itemPtr); itemPtr = nullptr; }
ITEM ITEM::split(double v)
{
	MediaItem * i = SplitMediaItem(itemPtr, v);
	if (i == nullptr)
		return itemPtr;

	return i;
}

ITEMLIST ITEM::split(vector<double> splitlist)
{
	ITEMLIST SplitItems(itemPtr);

	stable_sort(splitlist.begin(), splitlist.end(), [](double a, double b) { return a < b; });

	for (const auto& v : splitlist)
		SplitItems.push_back(SplitItems.back().split(v));

	return SplitItems;
}

int ITEM::getIndex() const { return GetMediaItemInfo_Value(itemPtr, "IP_ITEMNUMBER"); }
MediaTrack * ITEM::getTrack() const { return GetMediaItem_Track(itemPtr); }
int ITEM::getTrackIndex() const { return GetMediaTrackInfo_Value(GetMediaItem_Track(itemPtr), "IP_TRACKNUMBER"); }
double ITEM::getSnapOffset() const { return GetMediaItemInfo_Value(itemPtr, "D_SNAPOFFSET"); }
bool ITEM::isMuted() const { return 0.0 != GetMediaItemInfo_Value(itemPtr, "B_MUTE"); }
int ITEM::getGroupIndex() const { return GetMediaItemInfo_Value(itemPtr, "I_GROUPID"); }
double ITEM::getVolume() const { return GetMediaItemInfo_Value(itemPtr, "D_VOL"); }
double ITEM::getFadeInLen() const { return GetMediaItemInfo_Value(itemPtr, "D_FADEINLEN"); }
double ITEM::getFadeOutLen() const { return GetMediaItemInfo_Value(itemPtr, "D_FADEOUTLEN"); }
double ITEM::getFadeInLenAuto() const { return GetMediaItemInfo_Value(itemPtr, "D_FADEINLEN_AUTO"); }
double ITEM::getFadeOutLenAuto() const { return GetMediaItemInfo_Value(itemPtr, "D_FADEOUTLEN_AUTO"); }
int ITEM::getFadeInShape() const { return GetMediaItemInfo_Value(itemPtr, "C_FADEINSHAPE"); }
int ITEM::getFadeOutShape() const { return GetMediaItemInfo_Value(itemPtr, "C_FADEOUTSHAPE"); }
double ITEM::getFadeInCurve() const { return GetMediaItemInfo_Value(itemPtr, "D_FADEINDIR"); }
double ITEM::getFadeOutCurve() const { return GetMediaItemInfo_Value(itemPtr, "D_FADEOUTDIR"); }
bool ITEM::isSelected() const
{
	double value = GetMediaItemInfo_Value(itemPtr, "B_UISEL");
	return 0.0 != value;
}
int ITEM::getActiveTakeIndex() const { return GetMediaItemInfo_Value(itemPtr, "I_CURTAKE"); }

int ITEM::getNumTakes() { return CountTakes(itemPtr); }

double ITEM::getRate() const { return getActiveTake().getRate(); }

ITEM ITEM::duplicate() const
{
	char* chunk = GetSetObjectState((MediaItem*)itemPtr, "");

	ITEM copy = SplitMediaItem(itemPtr, getStart() + getLength() / 2);

	GetSetObjectState(copy.itemPtr, chunk);
	GetSetObjectState(itemPtr, chunk);
	FreeHeapPtr(chunk);

	UpdateArrange();

	return copy;
}

void ITEM::setTrackByIndex(int v)
{
	int tracks = CountTracks(0);
	for (int t = tracks; t < v + 1; ++t) InsertTrackAtIndex(t, true);
	MoveMediaItemToTrack(itemPtr, GetTrack(0, v));
}

bool ITEM::setTrack(MediaTrack * track) {
	bool t = MoveMediaItemToTrack(itemPtr, track);
	UpdateArrange();
	return t;
}
bool ITEM::setTrackByName(String name) { return MoveMediaItemToTrack(itemPtr, TRACK::getByName(name)); }
void ITEM::setActiveTake(int idx) { SetActiveTake(GetTake(itemPtr, idx)); }
void ITEM::setActiveTake(const TAKE & take) { SetActiveTake(take.getPointer()); }
void ITEM::setSnapOffset(double v) { SetMediaItemInfo_Value(itemPtr, "D_SNAPOFFSET", v); }
void ITEM::setMuted(bool v) { SetMediaItemInfo_Value(itemPtr, "B_MUTE", v); }
void ITEM::setVolume(double v) { SetMediaItemInfo_Value(itemPtr, "D_VOL", v); }
void ITEM::move(double v) { SetMediaItemInfo_Value(itemPtr, "D_POSITION", GetMediaItemInfo_Value(itemPtr, "D_POSITION") + v); }

bool ITEM::crop(RANGE r, bool move_edge)
{
	bool start_was_moved = false, end_was_moved = false;
	if (getStart() < r.start())
	{
		setStart(r.start());
		start_was_moved = true;
	}
	if (getEnd() > r.end())
	{
		setEnd(r.end());
		end_was_moved = true;
	}
	return start_was_moved || end_was_moved;
}

void ITEM::setFadeInLen(double v) { SetMediaItemInfo_Value(itemPtr, "D_FADEINLEN", v); }
void ITEM::setFadeOutLen(double v) { SetMediaItemInfo_Value(itemPtr, "D_FADEOUTLEN", v); }
void ITEM::setFadeInLenAuto(double v) { SetMediaItemInfo_Value(itemPtr, "D_FADEINLEN_AUTO", v); }
void ITEM::setFadeOutLenAuto(double v) { SetMediaItemInfo_Value(itemPtr, "D_FADEOUTLEN_AUTO", v); }
void ITEM::setFadeInShape(int v) { SetMediaItemInfo_Value(itemPtr, "C_FADEINSHAPE", v); }
void ITEM::setFadeOutShape(int v) { SetMediaItemInfo_Value(itemPtr, "C_FADEOUTSHAPE", v); }
void ITEM::setFadeInCurve(double v) { SetMediaItemInfo_Value(itemPtr, "D_FADEINDIR", v); }
void ITEM::setFadeOutCurve(double v) { SetMediaItemInfo_Value(itemPtr, "D_FADEOUTDIR", v); }
void ITEM::setSelected(bool v) { SetMediaItemInfo_Value(itemPtr, "B_UISEL", v); }

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
		for (auto& take : list)
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
		return getActiveTake().getTag(key);

	switch (iter->second)
	{
	case __name:
		return getActiveTake().getNameNoTags();
	case __track:
		if (get_value)
			return (String)TRACK(getTrack()).getIndex();
		else
			return TRACK(getTrack()).getName();
	case __length:
		return (String)getLength();
	case __rate:
		return (String)getRate();
	case __volume:
		return (String)getVolume();
	case __snapoffset:
		return (String)getSnapOffset();
	case __position:
		return (String)getStart();
	case __fadeinlen:
		return (String)getFadeInLen();
	case __fadeoutlen:
		return (String)getFadeOutLen();
	case __startoffset:
		return (String)getActiveTake().getStartOffset();
	case __tags:
		return getActiveTake().getTagString();
	case __pitch:
		return (String)getActiveTake().getPitch();
	case __file_extension:
		return getActiveTake().getFile().getFileExtension();
	case __noteclass:
		MIDI midi(getTag("n"));
		if (get_value)
			return (String)midi.getClassValue();
		else
			return midi.getClass(); 
	}

	return {};
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
		sort();
}

void ITEMLIST::collectSelectedItems()
{
	int items = CountSelectedMediaItems(0);

	if (items == 0)
		return;

	list.reserve(items);

	for (int i = 0; i < items; ++i)
		push_back(GetSelectedMediaItem(0, i));

	if (do_sort)
		sort();
}

double ITEMLIST::getStart() const { return front().getStart(); }
double ITEMLIST::getEnd() const { return back().getEnd(); }
double ITEMLIST::getSnapOffset() const { return front().getSnapOffset(); }
double ITEMLIST::getFadeInLen() const { return front().getFadeInLen(); }
double ITEMLIST::getFadeOutLen() const { return back().getFadeOutLen(); }
String ITEMLIST::GetPropertyStringFromKey(const String & key, bool use_value) const
{
	return front().GetPropertyStringFromKey(key, use_value);
}

bool ITEMLIST::isSelected() const
{
	for (const auto& i : list)
		if (!i.isSelected())
			return false;

	return true;
}

int ITEMLIST::crop(RANGE rng, bool move_edge)
{
	int num_cropped = 0;
	for (auto& i : list)
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

void ITEMLIST::setStart(double v)
{
	double posDiff = v - list[0].getStart();
	for (ITEM & item : list)
		item.move(posDiff);
}

void ITEMLIST::setEnd(double v)
{
	list.back().setEnd(v);
}

void ITEMLIST::setSnapOffset(double v)
{
	int i = 0;
	while (i < list.size() && !RANGE::is_touching(list[i], v))
		++i;
	if (i < list.size())
		list[i].setSnapOffset(v);
	else if (v < front().getStart())
		front().setSnapOffset(v);
	else
		list.back().setSnapOffset(v);
}

void ITEMLIST::move(double v)
{
	for (auto& i : list)
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

void ITEMGROUPLIST::collect_donotgroup(bool selected_only)
{
	ITEMLIST il;
	if (selected_only)
		il.collectSelectedItems();
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
		il.collectSelectedItems();
	else
		il.CollectItems();

	map<int, ITEMLIST> group_map;
	set<int> group_set;

	int non_group_counter = 0;

	for (ITEM & i : il)
	{
		int grp = i.getGroupIndex();
		if (grp == 0)
			grp = non_group_counter--;
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
		vector<ITEM> & il = selected_only ? t.ItemList_selected.list : t.list;
		ITEM previous_item = selected_only ? t.getSelectedItemList().front() : t.front();

		addNewList()->push_back(previous_item);

		for (const ITEM & i : il)
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

void ITEMGROUPLIST::collectSelectedItems(GROUPMODE group_mode)
{
	switch (group_mode)
	{
	case none: // do not group items
		collect_donotgroup(true);
		break;

	case grouped: // group grouped items
		collect_groupgrouped(true);
		break;

	case overlapping: // group overlapping items
		collect_groupoverlapping(true, true);
		break;

	case touching: // group touching items
		collect_groupoverlapping(true, false);
		break;
	}

	if (do_sort)
		sort();
}

void ITEMGROUPLIST::CollectItems(GROUPMODE group_mode)
{
	switch (group_mode)
	{
	case none: // do not group items
		collect_donotgroup(false);
		break;

	case grouped: // group grouped items
		collect_groupgrouped(false);
		break;

	case overlapping: // group overlapping items
		collect_groupoverlapping(false, true);
		break;

	case touching: // group touching items
		collect_groupoverlapping(false, false);
		break;
	}

	if (do_sort) sort();
}

int ITEMGROUPLIST::countItems()
{
	int c = 0;
	for (const auto& itemgroup : list)
		c += itemgroup.size();
	return c;
}
