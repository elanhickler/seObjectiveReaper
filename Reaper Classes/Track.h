#pragma once

class TRACK : public LIST<ITEM>, public OBJECT_NAMABLE, public OBJECT_VALIDATES
{
	friend class ITEMGROUPLIST;

public:
	// conversion
	static int count();
	static TRACK getByIndex(int getIndex);
	static TRACK getByName(const String & name);
	static TRACK getSelectedByIndex(int getIndex);
	static TRACK getSelectedByName(const String & name);
	static TRACK insertBeforeIndex(int i);
	static TRACK insertAfterIndex(int i);
	static TRACK insertFirst();
	static TRACK insertLast();

	// constructor
	TRACK() { track = nullptr; }
	TRACK(MediaTrack* track) : track(track) { TagManager.setStringWithTags(getName()); }
	TRACK(int getIndex)
	{
		track = GetTrack(0, getIndex);
		TagManager.setStringWithTags(getName());
	}



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
	TRACK getLastChild() const;
	TRACK getFirstChild() const;
	int folder() const { return GetMediaTrackInfo_Value(track, "I_FOLDERDEPTH"); }
	int countItems() const { return list.size(); }
	int countSelectedItems() const { return ItemList_selected.size(); }
	ITEM & getItem(int index) { return list[index]; }
	ITEM & getSelectedItem(int index) { return ItemList_selected[index]; }

	// setters
	void folder(int mode) { SetMediaTrackInfo_Value(track, "I_FOLDERDEPTH", mode); }
	void setAsLastFolder();

	 // functions
	void collectItems();
	void remove();
	void RemoveAllItemsFromProject();

	String getName() const override
	{
		char c[4096];
		GetSetMediaTrackInfo_String(track, "P_NAME", c, false);
		return c;
	}
	void setName(const String & v) override
	{
		GetSetMediaTrackInfo_String(track, "P_NAME", (char*)v.toRawUTF8(), true);
	}

	ITEMLIST & getSelectedItemList() { return ItemList_selected; }

	// getters
	bool sel() const { return GetMediaTrackInfo_Value(track, "I_SELECTED") == 1; }

	// setters
	void sel(bool state) { SetMediaTrackInfo_Value(track, "I_SELECTED", state); }

	// logic
	bool isValid() const { return track != nullptr; }
	bool is_selected() const { return sel(); }
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
	bool is_parent() const { return folder() == 1; }
	bool is_parent_of(const TRACK & t) { return track == t.getParent(); }
	bool is_child_of(const TRACK & t) { return getParent() == t; }

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
};

static TRACK INVALID_TRACK;

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
	TRACK & getSelectedByIdx(int getIndex);
	TRACK & getByName(const String & name);
};
