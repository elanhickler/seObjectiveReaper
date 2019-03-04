#include "ReaperClassesHeader.h"
#include "Item.h"

void ITEM::collectTakes()
{
	int num_takes = CountTakes(itemPtr);
	for (int i = 0; i < num_takes; ++i)
	{
		push_back(GetTake(itemPtr, i));
		back().itemParent = this;
	}
	OBJECT_NAMABLE::initialize();
}


// constructor

ITEM::ITEM(int i)
{
	itemPtr = GetMediaItem(0, i);
	OBJECT_NAMABLE::initialize();
}

ITEM::ITEM(MediaItem * item) : itemPtr(item)
{
	jassert(item != nullptr);
	collectTakes();
	OBJECT_NAMABLE::initialize();
}
ITEM::ITEM(MediaItem_Take * take)
{
	jassert(take != nullptr);
	itemPtr = GetMediaItemTake_Item(take);
	collectTakes();
	OBJECT_NAMABLE::initialize();
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

bool ITEM::isGrouped(const ITEM & i1, const ITEM & i2, bool must_be_on_same_track)
{
	if (must_be_on_same_track && i1.getTrack() != i2.getTrack()) return false;
	return i1.getGroupId() && i1.getGroupId() == i2.getGroupId();
}

ITEM ITEM::get(int idx) { return GetMediaItem(0, idx); }
ITEM ITEM::getSelected(int idx) { return GetSelectedMediaItem(0, idx); }

ITEM ITEM::createBlank(MediaTrack * track, double position, double length)
{
	ITEM newItem(AddMediaItemToTrack(track));
	newItem.setPosition(position);
	newItem.setLength(length);
	return std::move(newItem);
}

ITEM ITEM::createMidi(MediaTrack * track, double position, double length)
{
	ITEM item = CreateNewMIDIItemInProj(track, position, position + length, 0);
	return item;
}

String ITEM::getObjectName() const { return getActiveTake().getName(); }
void ITEM::setObjectName(const String & v)
{
	getActiveTake().setName(v);
}
double ITEM::getStart() const { return GetMediaItemInfo_Value(itemPtr, "D_POSITION"); }

void ITEM::setStart(double v)
{
	PROJECT::unselectAllItems();
	SetMediaItemInfo_Value(itemPtr, "B_UISEL", true);
	//NUDGE::apply(v, false);
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
int ITEM::getGroupId() const { return GetMediaItemInfo_Value(itemPtr, "I_GROUPID"); }
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
void ITEM::setGroupId(int v)
{
	SetMediaItemInfo_Value(itemPtr, "I_GROUPID", double(v));
}
void ITEM::removeGroup()
{
	setGroupId(0);
}
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
		return getActiveTake().getNameTagsOnly();
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

void ITEMLIST::collectItems()
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

double ITEMLIST::getStart() const
{
	if (do_sort)
	{
		return front().getStart();
	}
	else
	{
		double start = list[0].getStart();
		for (const auto & item : list)
			start = min(start, item.getStart());
		return start;
	}
}
double ITEMLIST::getEnd() const
{
	double end = list[0].getEnd();
	for (const auto & item : list)
		end = max(end, item.getEnd());
	return end;
}
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
		il.collectItems();

	push_back(il);

	reserve(il.size());
}

void ITEMGROUPLIST::collect_groupgrouped(bool selected_only)
{
	int items = selected_only ? CountSelectedMediaItems(0) : CountMediaItems(0);
	reserve(items);

	ITEMLIST itemList;
	if (!do_sort)
		itemList.disableSort();

	if (selected_only)
		itemList.collectSelectedItems();
	else
		itemList.collectItems();

	map<int, ITEMLIST> group_map;
	Array<int> group_order_appearance;

	int non_group_counter = 0;
	for (ITEM& item : itemList)
	{
		int grp;
		if (item.getGroupId() == 0)
		{
			grp = non_group_counter--;
			group_order_appearance.add(grp);
		}
		else
		{
			grp = item.getGroupId();
			group_order_appearance.addIfNotAlreadyThere(grp);
		}

		group_map[grp].push_back(item);
	}

	for (int id : group_order_appearance)
	{
		push_back(group_map[id]);
		back().groupId = id;
	}
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

			bool do_new_list = !(must_be_overlapping ? RANGE::is_overlapping(previous_item, i) : RANGE::is_touching(previous_item, i));

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

	if (do_sort)
		sort();
}

int ITEMGROUPLIST::countItems()
{
	int c = 0;
	for (const auto& itemgroup : list)
		c += itemgroup.size();
	return c;
}

double AUDIOFUNCTION::getPeak(TAKE & take, double * frameIndexOut, double * channelIndexOut)
{
	double peakValue = 0;
	double absPeakValue = 0;
	int frameIndexForPeak = 0;
	int channelIndexForPeak = take.getFirstChannel();

	auto func = [&](int ch, int fr)
	{
		double value = take[ch][fr];
		double absValue = abs(value);

		if (absValue > absPeakValue)
		{
			peakValue = value;
			absPeakValue = absValue;
			frameIndexForPeak = fr;
			channelIndexForPeak = ch;

			if (absPeakValue == 1)
				return;
		}
	};

	for (int fr = 0; fr < take.getNumFrames(); ++fr)
		for (int ch = take.getFirstChannel(); ch < take.getLastChannel(); ++ch)
			func(ch, fr);				

	if (frameIndexOut)
		*frameIndexOut = frameIndexForPeak;
	if (channelIndexOut)
		*channelIndexOut = channelIndexForPeak;
	return peakValue;
}

vector<double> AUDIOFUNCTION::sumChannelModeChannels(TAKE & take)
{
	vector<double> summed(take.getNumFrames(), 0);

	if (take.getNumChannelModeChannels() == 1)
		return take.getAudioChannel(take.getFirstChannel());

	for (int fr = 0; fr < take.getNumFrames(); ++fr)
		for (int ch = take.getFirstChannel(); ch < take.getLastChannel(); ++ch)
			summed[fr] += take[ch][fr];

	return std::move(summed);
}

vector<double> AUDIOFUNCTION::sumAllChannels(TAKE & take)
{
	vector<double> summed(take.getNumFrames(), 0);

	if (take.getNumChannels() == 1)
		return take.getAudioChannel(0);

	for (int fr = 0; fr < take.getNumFrames(); ++fr)
		for (int ch = 0; ch < take.getNumChannels(); ++ch)
			summed[fr] += take[ch][fr];

	return std::move(summed);
}

double AUDIOFUNCTION::getPeakRMS(TAKE & take, double timeWindowForPeakRMS)
{
	auto summedAudio = sumChannelModeChannels(take);
	int numChannels = take.getNumChannelModeChannels();

	if (numChannels > 1)
		for (auto & value : summedAudio)
			value /= double(numChannels);
	
	return RAPT::getMaxShortTimeRMS<double>(summedAudio.data(), summedAudio.size(), take.getSampleRate()*timeWindowForPeakRMS);
}

bool AUDIOFUNCTION::isAudioSilent(TAKE & take, double minimumAmplitude)
{
	for (int ch = take.getFirstChannel(); ch < take.getLastChannel(); ++ch)
		for (int fr = 0; fr < take.getNumFrames(); ++fr)
			if (take.getSample(ch, fr) >= minimumAmplitude)
				return false;
	return true;
}

void AUDIOPROCESS::processTakeList(TAKELIST & list, std::function<void(TAKE&)> perTakeFunction)
{
	prepareToStart();

	for (auto & take : list)
	{
		loadTake(take);

		perTakeFunction(take);

		unloadTake(take);
	}

	prepareToEnd();
}

void AUDIOPROCESS::prepareToStart()
{
	PROJECT::setAllItemsOffline();
	PROJECT::saveItemSelection();
	PROJECT::unselectAllItems();
}

void AUDIOPROCESS::prepareToEnd()
{
	PROJECT::loadItemSelection();
	PROJECT::setAllItemsOnline();
}

void AUDIOPROCESS::loadTake(TAKE & take)
{
	PROJECT::selectItem(take.getMediaItemPtr());
	PROJECT::setSelectedItemsOnline();
	take.initAudio();
	take.loadAudio();
}

void AUDIOPROCESS::unloadTake(TAKE & take)
{
	take.unloadAudio();
	PROJECT::setSelectedItemsOffline();
	PROJECT::unselectItem(take.getMediaItemPtr());
}

TAKE::TAKE(const vector<vector<double>> & multichannelAudio, FILE fileToWriteTo)
{

}
