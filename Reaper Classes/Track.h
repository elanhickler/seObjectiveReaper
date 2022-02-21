#pragma once

class TRACKLIST;

class TRACK : public LIST<ITEM>, public OBJECT_NAMABLE, public OBJECT_VALIDATES
{
	friend class ITEMGROUPLIST;

public:
	// conversion
	static int count() { return CountTracks(nullptr); }
	static TRACK getMaster();
	static TRACK getByIndex(int getIndex);
	static TRACKLIST getByName(const String & name);
	static TRACK getSelectedByIndex(int getIndex);
	static TRACKLIST getSelectedByName(const String & name);
	static TRACK insertBeforeIndex(int i);
	static TRACK insertAfterIndex(int i);
	static TRACK insertFirst();
	static TRACK insertLast();
	static TRACK insertAsLastChildOf(TRACK& parent, const String& nameForNewTrack)
	{
		auto childTrack = parent.getLastChild();

		if (!childTrack.isValid())
		{
			auto t = insertAfterIndex(parent.getIndex());
			parent.setAsFolderParent();
			t.setAsLastChild();
			t.setName(nameForNewTrack);
			return t;
		}
		else
		{
			int i = childTrack.getIndex() + 1;
			InsertTrackAtIndex(i, true);

			int folderDepth = TRACK(i - 1).getFolderDepth();
			TRACK(i - 1).setFolderDepth(0);
			TRACK(i).setFolderDepth(folderDepth);

			TrackList_AdjustWindows(false);

			TRACK(i).setName(nameForNewTrack);
			return TRACK(i);
		}
	}

	// constructor
	TRACK() { track = nullptr; }
	TRACK(MediaTrack* track) : track(track) { OBJECT_NAMABLE::initialize(); }
	TRACK(int getIndex) : track(GetTrack(0, getIndex)) { OBJECT_NAMABLE::initialize(); }

	// conversion
	operator void*() const { return track; }
	operator MediaTrack*() const { return track; }

	// operators
	bool operator==(MediaTrack * rhs) const { return track == rhs; }
	bool operator!=(MediaTrack * rhs) const { return track != rhs; }
	bool operator==(const TRACK & rhs) const { return track == rhs.track; }
	bool operator!=(const TRACK & rhs) const { return track != rhs.track; }

	// getters
	MediaTrack * getPointer() { return track; }
	int getIndex() const { return GetMediaTrackInfo_Value(track, "IP_TRACKNUMBER") - 1; }
	TRACK getParent() const;
	TRACK getParentWithName(const String& v) const; // recursively searches all levels of parents
	TRACK getChildWithName(const String& v) const; // recursively searchs all levels of children
	TRACK getLastChild() const;
	TRACK getFirstChild() const;
	int getFolderDepth() const { return GetMediaTrackInfo_Value(track, "I_FOLDERDEPTH"); }
	int countItems() const { return (int)list.size(); }
	int countSelectedItems() const { return (int)ItemList_selected.size(); }
	ITEM & getItem(int index) { return list[index]; }
	ITEM & getSelectedItem(int index) { return ItemList_selected[index]; }

	// setters
	void setIndex() {}
	void setSelected(bool state) { SetMediaTrackInfo_Value(track, "I_SELECTED", state); }
	void setFolderDepth(int mode);
	void setAsFolderParent();
	void setAsChild();
	void setAsLastChild();

	 // functions
	// Populates the track object's item list and selected item list.
	void collectItems();
	void remove();
	void RemoveAllItemsFromProject();

	ITEMLIST& getItemList() { return ItemList_all; }
	ITEMLIST& getSelectedItemList() { return ItemList_selected; }

	// setters


	// logic
	bool isValid() const override { return track != nullptr && getIndex() >= 0; }
	bool is_selected() const { return GetMediaTrackInfo_Value(track, "I_SELECTED") == 1; }
	bool hasMidiItems() const
	{
		for (const ITEM & item : ItemList_selected)
			if (item.getActiveTake().isMidi())
				return true;
		return false;
	}
	bool has_items() const { return countItems() > 0; }
	bool has_selected_items() const { return countSelectedItems() > 0; }
	bool has_child() const { return TRACK(getIndex() + 1).getParent() == track; }
	bool is_last_child() const
	{
		return TRACK(getIndex() + 1).is_parent() || getFolderDepth() < 0;
	}
	bool has_parent() const {
		TRACK t = GetParentTrack(track);
		return t.getPointer() != track;
	}
	bool is_parent() const { return getFolderDepth() == 1; }
	bool is_parent_of(const TRACK & t) { return track == t.getParent(); }
	bool is_child_of(const TRACK & t) { return getParent() == t; }
	bool is_last_track() { return getIndex() == (count() - 1); }
	bool is_first_track() { return getIndex() == 0; }


	enum
	{
		__name,
		__tags,
	};

	map<String, int> method_lookup = {
			{ "N", __name },
			{ "t", __tags },
	};
	String GetPropertyStringFromKey(const String & key, bool get_value = false) const override;

protected:
	// member
	MediaTrack* track;
	ITEMLIST ItemList_all;
	ITEMLIST ItemList_selected;

	String getObjectName() const override
	{
		char c[4096];
		GetSetMediaTrackInfo_String(track, "P_NAME", c, false);
		return c;
	}
	void setObjectName(const String & v) override
	{
		GetSetMediaTrackInfo_String(track, "P_NAME", (char*)v.toRawUTF8(), true);
	}
};

class TRACKLIST : public LIST<TRACK>
{
public:
	static TRACKLIST CreateTrackAsFirstChild(TRACK parent, int how_many = 1);
	static TRACKLIST CreateTrackAsLastChild(TRACK parent, int how_many = 1);
	static TRACKLIST GetSelected();
	static TRACKLIST Get();
	static TRACKLIST GetChildren(TRACK parent);
	static TRACKLIST GetParentsOfSelected();
	static TRACK getSelectedChildByIdx(TRACK parent, int getIndex);
	static int CountChildren(TRACK parent);

	TRACKLIST() {};
	TRACKLIST(TRACK t) { push_back(t); };

	// project
	void CollectTracks();
	void CollectSelectedTracks();
	void CollectTracksWithItems();
	void CollectTracksWithSelectedItems();

	// getters
	TRACK getSelectedByIdx(int getIndex);
	TRACK getByName(const String & name);
};
