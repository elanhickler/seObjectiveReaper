#pragma once

class TRACK : public LIST<ITEM>, public OBJECT_NAMABLE, public OBJECT_VALIDATES
{
	friend class ITEMGROUPLIST;

public:
	// conversion
	static int count();
	static TRACK getByIndex(int idx);
	static TRACK getByName(const String & name);
	static TRACK getSelectedByIndex(int idx);
	static TRACK getSelectedByName(const String & name);
	static TRACK insertBeforeIndex(int i);
	static TRACK insertAfterIndex(int i);

	// constructor
	TRACK() { track = nullptr; }
	TRACK(MediaTrack* track) : track(track) { TagManager.setStringWithTags(getName()); }
	TRACK(int idx)
	{
		track = GetTrack(0, idx);
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
	int idx() const { return GetMediaTrackInfo_Value(track, "IP_TRACKNUMBER") - 1; }
	TRACK parent() const;
	TRACK getLastChild() const;
	TRACK getFirstChild() const;
	int folder() const { return GetMediaTrackInfo_Value(track, "I_FOLDERDEPTH"); }
	ITEM & getSelectedItem(int idx) { return ItemList_selected[idx]; }
	int countItems() const { return list.size(); }
	int countSelectedItems() const { return ItemList_selected.size(); }

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

	ITEMLIST getItemList() { return ItemList_all; }
	ITEMLIST getSelectedItemList() { return ItemList_selected; }

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
			if (item.getActiveTake()->isMidi())
				return true;
		return false;
	}
	bool has_items() const { return countItems() > 0; }
	bool has_selected_items() const { return countSelectedItems() > 0; }
	bool has_child() const { return TRACK(idx() + 1).parent() == track; }
	bool is_parent() const { return folder() == 1; }
	bool is_parent_of(const TRACK & t) { return track == t.parent(); }
	bool is_child_of(const TRACK & t) { return parent() == t; }

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
static TRACK TRACK_invalid;

class TRACKLIST : public LIST<TRACK>
{
public:
	static TRACKLIST CreateTrackAsFirstChild(TRACK parent, int how_many = 1);
	static TRACKLIST CreateTrackAsLastChild(TRACK parent, int how_many = 1);
	static TRACKLIST GetSelected();
	static TRACKLIST Get();
	static TRACKLIST GetChildren(TRACK parent);
	static TRACKLIST GetParentsOfSelected();
	static TRACK getSelectedChildByIdx(TRACK parent, int idx);
	static int CountChildren(TRACK parent);

public:
	TRACKLIST() {};
	TRACKLIST(TRACK t) { push_back(t); };

	// project
	void CollectTracks();
	void CollectSelectedTracks();
	void CollectTracksWithItems();
	void CollectTracksWithSelectedItems();

	// getters
	TRACK & getSelectedByIdx(int idx);
	TRACK & getByName(const String & name);
};
