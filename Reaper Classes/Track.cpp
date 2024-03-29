#include "ReaperClassesHeader.h"
#include "Track.h"
#include "Item.h"


// conversion

TRACK TRACK::getMaster()
{
	return TRACK(GetMasterTrack(nullptr));
}

TRACK TRACK::getByIndex(int idx) { return TRACK(idx); }

TRACKLIST TRACK::getByName(const String& name)
{
	int num_tracks = CountTracks(0);
	TRACKLIST tracklistOut;
	for (int i = 0; i < num_tracks; ++i)
	{
		TRACK t = GetTrack(0, i);
		if (name == t.getName())
			tracklistOut.push_back(t);
	}
	return tracklistOut;
}

TRACK TRACK::getSelectedByIndex(int idx)
{
	return GetSelectedTrack(0, idx);
}

TRACKLIST TRACK::getSelectedByName(const String & name)
{
	int num_tracks = CountSelectedTracks(0);
	TRACKLIST tracklistOut;
	for (int i = 0; i < num_tracks; ++i)
	{
		TRACK t = GetSelectedTrack(0, i);
		if (name == t.getName())
			tracklistOut.push_back(t);
	}
	return tracklistOut;
}

TRACK TRACK::insertBeforeIndex(int i)
{
	InsertTrackAtIndex(i, true);
	TrackList_AdjustWindows(false);
	return TRACK(i);
}
TRACK TRACK::insertAfterIndex(int i)
{
	InsertTrackAtIndex(i + 1, true);
	TrackList_AdjustWindows(false);
	return TRACK(i + 1);
}
TRACK TRACK::insertFirst() { InsertTrackAtIndex(0, true); TrackList_AdjustWindows(false); return TRACK(0); }
TRACK TRACK::insertLast() { InsertTrackAtIndex(GetNumTracks(), true); TrackList_AdjustWindows(false); return TRACK(GetNumTracks() - 1); }

TRACK TRACK::getParent() const
{
	TRACK t = GetParentTrack(track);

	if(t.track == track)
		t.track = nullptr;

	return t;
}

TRACK TRACK::getParentWithName(const String& v) const
{
	auto parentTrack = getParent();
	while (parentTrack.isValid())
	{
		if (parentTrack.getName() == v)
			break;

		parentTrack = parentTrack.getParent();
	}

	return parentTrack;
}

TRACK TRACK::getChildWithName(const String& v) const
{
	int numTracks = CountTracks(nullptr);

	for (int i = getIndex() + 1; i < numTracks; ++i)
	{
		auto t = TRACK(i);
		if (t.getParent().getPointer() == track)
			if (t.getName() == v)
				return t;
	}

	return {};
}

TRACK TRACK::getLastChild() const
{
	if (!has_child())
		return {};
	
	int numTracks = CountTracks(nullptr);

	int i = getIndex() + 1;
	int i2;

	for (i2 = i; i2 < numTracks; ++i2)
		if (TRACK(i2).getParent() == track)
			i = i2;

	return TRACK(i);

}

TRACK TRACK::getFirstChild() const
{
	TRACK supposed_child(getIndex() + 1);
	if (supposed_child.is_child_of(track))
		return supposed_child;
	return {};
}


// setters

void TRACK::setFolderDepth(int mode) { SetMediaTrackInfo_Value(track, "I_FOLDERDEPTH", mode); }

void TRACK::setAsFolderParent() { SetMediaTrackInfo_Value(track, "I_FOLDERDEPTH", 1); }

void TRACK::setAsChild() { SetMediaTrackInfo_Value(track, "I_FOLDERDEPTH", 0); }

void TRACK::setAsLastChild()
 {
 	TRACK t = getParent();
	int i = 0;
	while (t.isValid())
	{
		t = t.getParent();
		--i;
	}
	setFolderDepth(i);
}

void TRACK::collectItems()
{
	list.clear();
	ItemList_selected.clear();

	int num_items = CountTrackMediaItems(track);

	for (int i = 0; i < num_items; ++i)
	{
		list.push_back(GetTrackMediaItem(track, i));

		if (ITEM(GetTrackMediaItem(track, i)).isSelected())
			ItemList_selected.push_back(list.back());
	}
}

void TRACK::remove()
{
	auto prev_track = TRACK(getIndex() - 1);
	if (!prev_track.is_parent() && prev_track.isValid())
		prev_track.setFolderDepth(getFolderDepth());
	DeleteTrack(track);
	track = nullptr;
}

void TRACK::RemoveAllItemsFromProject()
{
	for (ITEM & item : list)
		item.remove();
}

TRACKLIST TRACKLIST::CreateTrackAsFirstChild(TRACK parent, int how_many)
{
	bool did_not_have_child = !parent.has_child();
	parent.setFolderDepth(1);

	TRACKLIST tl;

	for (int i = 0; i < how_many; ++i)
		tl.push_back(TRACK::insertAfterIndex(parent.getIndex() + i));

	if (did_not_have_child)
		TRACK(parent.getIndex() + how_many).setAsLastChild();

	return tl;
}

TRACKLIST TRACKLIST::CreateTrackAsLastChild(TRACK parent, int how_many)
{
	TRACKLIST tl;

	if (how_many == 0) return tl;

	auto t = parent.getLastChild();

	int orig_folder_mode = t.getFolderDepth();
	t.setFolderDepth(0);

	for (int i = 0; i < how_many; ++i)
		tl.push_back(TRACK::insertAfterIndex(t.getIndex() + i));

	tl.back().setFolderDepth(orig_folder_mode);

	return tl;
}

TRACKLIST TRACKLIST::GetSelected()
{
	TRACKLIST list;
	int num_tracks = CountSelectedTracks(0);
	for (int t = 0; t < num_tracks; ++t)
		list.push_back(GetSelectedTrack(0, t));
	return list;
}

TRACKLIST TRACKLIST::Get()
{
	TRACKLIST list;
	int num_tracks = CountTracks(0);
	for (int t = 0; t < num_tracks; ++t)
		list.push_back(GetTrack(0, t));
	return list;
}

TRACKLIST TRACKLIST::GetChildren(TRACK parent)
{
	TRACKLIST TrackList;

	int i = parent.getIndex() + 1;
	TRACK t(i);
	while (parent.is_parent_of(t))
	{
		TrackList.push_back(t);
		t = TRACK(++i);
	}

	return TrackList;
}

TRACKLIST TRACKLIST::GetParentsOfSelected()
{
	auto TrackList = TRACKLIST::GetSelected();
	TRACKLIST ParentTracks;

	for (TRACK & track : TrackList)
	{
		if (ParentTracks.has(track.getParent())) continue; //ensure you don't add the same track twice
		TRACK t = track.getParent();
		if (!t.isValid()) continue;
		ParentTracks.push_back(t);
	}

	return ParentTracks;
}

int TRACKLIST::CountChildren(TRACK parent)
{
	int count = 0;

	int i = parent.getIndex();
	while (TRACK(++i).is_parent_of(parent))
		++count;

	return count;
}

void TRACKLIST::CollectTracks()
{
	int num_tracks = CountTracks(0);
	for (int t = 0; t < num_tracks; ++t)
		push_back(GetTrack(0, t));
}

void TRACKLIST::CollectSelectedTracks()
{
	int num_tracks = CountSelectedTracks(0);
	for (int t = 0; t < num_tracks; ++t)
		push_back(GetSelectedTrack(0, t));
}

void TRACKLIST::CollectTracksWithItems()
{
	int num_tracks = CountTracks(0);
	for (int t = 0; t < num_tracks; ++t)
	{
		TRACK track = GetTrack(0, t);
		track.collectItems();
		if (track.has_items())
			push_back(track);
	}
}

void TRACKLIST::CollectTracksWithSelectedItems()
{
	int num_tracks = CountTracks(0);
	for (int t = 0; t < num_tracks; ++t)
	{
		TRACK track = GetTrack(0, t);
		track.collectItems();
		if (track.has_selected_items())
			push_back(track);
	}
}

TRACK TRACKLIST::getByName(const String & name)
{
	for (auto& t : list)
		if (name == t.getName())
			return t;

	return TRACK();
}

TRACK TRACKLIST::getSelectedByIdx(int idx)
{
	int sel_idx = 0;
	for (auto& track : list)
	{
		if (track.is_selected())
		{
			++sel_idx;
			if (sel_idx == idx)
				return track;
		}
	}

	return TRACK();
}

TRACK TRACKLIST::getSelectedChildByIdx(TRACK parent, int idx)
{
	TRACKLIST TrackList;

	int i = parent.getIndex() + 1;
	int count = -1;
	TRACK t(i);
	while (t.is_parent_of(parent))
	{
		if (t.is_selected())
			++count;
		if (count == idx)
			return t;
		t = TRACK(++i);
	}

	return {};
}

String TRACK::GetPropertyStringFromKey(const String & key, bool get_value) const
{
	auto iter = method_lookup.find(key);

	if (iter == method_lookup.end())
		return getTag(key);

	switch (iter->second)
	{
	case __name:
		if (get_value)
			return String(getIndex());
		return getNameNoTags();
	case __tags:
		return getNameTagsOnly();
	}

	return {};
}

ITEM ITEM::createFromAudioData(const AUDIODATA & audioData, const File & fileToWriteTo, const TRACK & existingTrack, double startTime)
{
	jassert(existingTrack.isValid()); // track must exist in the REAPER project

	try
	{
		FileHelper::createFile(fileToWriteTo.getFullPathName(), true);
	}
	catch (std::exception&) {}

	audioData.writeToFile(fileToWriteTo);

	auto newItem = ITEM::createBlank(existingTrack, startTime, audioData.getLength());
	newItem.setName(fileToWriteTo.getFileNameWithoutExtension());
	newItem.getActiveTake().setFile(fileToWriteTo);
	return newItem;
}

int ITEM::count()
{
	return CountMediaItems(nullptr);
}

int ITEM::countSelected()
{
	return CountSelectedMediaItems(nullptr);
}
