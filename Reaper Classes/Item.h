#pragma once

enum GROUPMODE { none, grouped, overlapping, touching };

class ITEM : public LIST<TAKE>, public OBJECT_MOVABLE, public OBJECT_NAMABLE, public OBJECT_VALIDATES
{
	friend class ITEMLIST;
	friend class TagParser;

public:
	static bool IsGrouped(const ITEM & i1, const ITEM & i2, bool must_be_on_same_track = true);
	static ITEM get(int idx);
	static ITEM getSelected(int idx);
	static ITEM createMidi(MediaTrack * track, double position, double length);

	enum FADESHAPE
	{
		LIN,
		LOG,
		EXPO,
		LOG_2,
		EXPO_2,
		SCURV,
		SCURV_2
	};

public:
	// constructor
	ITEM();
	ITEM(int i) { itemPtr = GetMediaItem(0, i); }
	ITEM(MediaItem * item);
	ITEM(MediaItem_Take * take);

	// conversion
	operator void*() const { return itemPtr; }
	operator MediaItem*() const { return itemPtr; }
	operator MediaItem_Take*() const { return getActiveTake().getPointer(); }

	// operator
	bool operator==(const MediaItem * rhs) const { return itemPtr == rhs; }
	bool operator!=(const MediaItem * rhs) const { return itemPtr != rhs; }
	bool operator==(const ITEM & rhs) const { return itemPtr == rhs.itemPtr; }
	bool operator!=(const ITEM & rhs) const { return itemPtr != rhs.itemPtr; }

	/* FUNCTIONS */

	// copies the item exactly taking the same position and length, thereby overlapping 100%
	ITEM duplicate() const;
	void remove();
	void move(double v);
	bool crop(RANGE r, bool move_edge);

	// returns the item created from the right-hand-side of the split, or invalid item if split failed
	ITEM split(double v);

	// returns ITEMLIST of items created from splitting including itself using global time.
	ITEMLIST split(vector<double> splitlist);

	MediaItem* getPointer() { return itemPtr; }

	/* GETTER */
	String getName() const override;
	void setName(const String & v) override;

	bool isValid() const override;

	int getActiveTakeIndex() const;
	const TAKE & getActiveTake() const;
	TAKE & getActiveTake();
	const TAKE & getTake(int i) const;
	TAKE & getTake(int i);
	int getNumTakes();

	MediaTrack* getTrack() const;
	int getTrackIndex() const;

	bool isMuted() const;
	bool isSelected() const;

	int getIndex() const;
	int getGroupIndex() const;
	double getVolume() const;
	double getRate() const;
	double getSnapOffset() const;

	double getFadeInLen() const;
	double getFadeOutLen() const;
	double getFadeInLenAuto() const;
	double getFadeOutLenAuto() const;
	int getFadeInShape() const;
	int getFadeOutShape() const;
	double getFadeInCurve() const;
	double getFadeOutCurve() const;

	/* SETTER */
	void setMuted(bool v);
	void setSelected(bool v);

	void setActiveTake(int idx);
	void setActiveTake(const TAKE & take);

	bool setTrack(MediaTrack* track);
	void setTrackByIndex(int v);
	bool setTrackByName(String name);

	void setVolume(double v);
	void setRate(double new_rate, bool warp = true);
	void setSnapOffset(double v);

	void setFadeInLen(double v);
	void setFadeOutLen(double v);
	void setFadeInLenAuto(double v);
	void setFadeOutLenAuto(double v);
	void setFadeInShape(int v);
	void setFadeOutShape(int v);
	void setFadeInCurve(double v);
	void setFadeOutCurve(double v);

	double getStart() const override;
	void setStart(double v) override;

	double getEnd() const override;
	void setEnd(double v) override;

	double getLength() const override;
	void setLength(double v) override;

	void setPosition(double v) override;

	Colour getColor() const override;
	void setColor(Colour v) override;

protected:
	MediaItem* itemPtr;

	void collectTakes();

	enum
	{
		__name,
		__track,
		__length,
		__rate,
		__volume,
		__snapoffset,
		__position,
		__fadeinlen,
		__fadeoutlen,
		__startoffset,
		__tags,
		__pitch,
		__file_extension
	};

	map<String, int> method_lookup = {
			{ "N", __name },
			{ "T", __track },
			{ "L", __length },
			{ "R", __rate },
			{ "V", __volume },
			{ "SN", __snapoffset },
			{ "PP", __position },
			{ "FI", __fadeinlen },
			{ "FO", __fadeoutlen },
			{ "SO", __startoffset },
			{ "t", __tags },
			{ "ext", __file_extension }
	};

	String GetPropertyStringFromKey(const String & key, bool get_value = false) const override;
};

class ITEMLIST : public LIST<ITEM>, public OBJECT_MOVABLE
{
public:
	static int CountSelected();
	static int Count();
protected:
	int m_group;

public:
	// constructor
	ITEMLIST() {}
	ITEMLIST(ITEM i)
	{
		push_back(i);
	}

	ITEMLIST(vector<ITEM> v)
	{
		list = v;
	}

	operator MediaItem*() { return list[0]; }
	operator ITEM() { return list[0]; }
	ITEMLIST operator=(vector<ITEM> rhs) { list = rhs; return *this; }

	void CollectItems();
	void collectSelectedItems();

	// functions
	void move(double v);
	void remove();
	int crop(RANGE r, bool move_edge);

	// getters
	double getStart() const override;
	double getEnd() const override;
	double getSnapOffset() const;
	double getFadeInLen() const;
	double getFadeOutLen() const;
	String GetPropertyStringFromKey(const String & key, bool use_value) const;
	bool isSelected() const;

	// setters
	int setTrack(MediaTrack* track);
	void setStart(double v) override;
	void setEnd(double v) override;
	void setSnapOffset(double v);
	void setSelected(bool select);
};

class ITEMGROUPLIST : public LIST<ITEMLIST>
{
protected:
	//function
	void collect_donotgroup(bool selected_only);
	void collect_groupgrouped(bool selected_only);
	void collect_groupoverlapping(bool selected_only, bool must_be_overlapping);

public:
	// constructor
	ITEMGROUPLIST() {}
	ITEMGROUPLIST(ITEMLIST ItemList) { push_back(ItemList); }

	// project
	void CollectItems(GROUPMODE group_mode);
	void collectSelectedItems(GROUPMODE group_mode);

	// actions
	ITEMLIST * addNewList()
	{
		list.push_back(ITEMLIST());
		return &list.back();
	}

	operator ITEMLIST() { return list[0]; }

	// mode 0 = do not group items
	// mode 1 = group grouped items
	// mode 2 = group overlapping items
	// mode 3 = group touching items

	// functions
	int countItems();
};
