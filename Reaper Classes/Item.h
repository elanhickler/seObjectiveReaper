#pragma once

/*
none: All items treated as single items and put into individual groups
grouped: Items are grouped according to REAPER item group feature.
overlapping: Items are grouped if they are overlapping. Items simply touching will not be grouped.
touching: Items are grouped if they are right up against eachother or overlapping.
*/
enum GROUPMODE { none, grouped, overlapping, touching };

class TRACK;
class ITEMLIST;

class ITEM : public TAKELIST, public OBJECT_MOVABLE, public OBJECT_NAMABLE, public OBJECT_VALIDATES
{
	friend class ITEMLIST;
	friend class TagParser;

public:
	static bool isGrouped(const ITEM & i1, const ITEM & i2, bool must_be_on_same_track = true);
	static ITEM get(int idx);
	static ITEM getSelected(int idx);
	static ITEM createBlank(MediaTrack * track, double position, double length);
	static ITEM createMidi(MediaTrack * track, double position, double length);
	static ITEM createFromAudioData(const AUDIODATA & audioFile, const File & fileToWriteTo, const TRACK & existingTrack, double startTime);
	static int count();

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
	ITEM() {}
	ITEM(int i);
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

	// Copies the item exactly taking the same position, length, and properties, thereby overlapping 100%
	ITEM copy() const;
	void remove();
	void move(double v);

	// Returns a new item object as the original item object is deleted. Time is local to item.
	ITEM crop(double start, double end);

	// Returns the right hand side item of the crop, the original item object is kept. Time is local to item;
	ITEM invertCrop(double start, double end);

	// Returns the item created from the right-hand-side of the split, or invalid item if split failed
	ITEM split(double v);

	// Returns ITEMLIST of items created from splitting including itself using global time.
	ITEMLIST split(vector<double> splitlist);

	MediaItem* getPointer() { return itemPtr; }

	/* GETTER */

	bool isValid() const override;

	int getActiveTakeIndex() const;
	const TAKE & getActiveTake() const;
	TAKE & getActiveTake();
	const TAKE & getTake(int i) const;
	TAKE & getTake(int i);
	int getNumTakes();
	TAKELIST & getTakeList() { return *this; }

	MediaTrack* getTrack() const;
	int getTrackIndex() const;

	bool isMuted() const;
	bool isSelected() const;

	int getIndex() const;
	int getGroupId() const;
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
	void rateStretchToTime(double time);
	void setSnapOffset(double v);

	void setGroupId(int v);
	void removeGroup();

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
	String getObjectName() const override;
	void setObjectName(const String & v) override;

	MediaItem* itemPtr = nullptr;

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
		__file_extension,
		__noteclass
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
			{ "ext", __file_extension },
			{ "noteclass", __noteclass },
	};

	String GetPropertyStringFromKey(const String & key, bool get_value = false) const override;
};

class ITEMLIST : public LIST<ITEM>, public OBJECT_MOVABLE
{
	friend class ITEMGROUPLIST;

public:
	// constructor
	ITEMLIST() {}
	ITEMLIST(ITEM i)
	{
		push_back(i);
	}

	ITEMLIST(const vector<ITEM> & v)
	{
		list = v;
	}
  
  virtual ~ITEMLIST(){}

	operator MediaItem*() { return list[0]; }
	operator ITEM() { return list[0]; }
	ITEMLIST operator=(vector<ITEM> rhs) { list = rhs; return *this; }

	void collectItems();
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
	TAKELIST & getActiveTakeList()
	{
		if (TakeList.size() != list.size())
		{
			TakeList.clear();
			for (const auto & item : list)
				TakeList.push_back(item.getActiveTake());
		}

		return TakeList;
	}

	// setters
	int setTrack(MediaTrack* track);
	void setStart(double v) override;
	void setPosition(double v) override { setStart(v); }
	void setEnd(double v) override;
	void setSnapOffset(double v);
	// Sets selection flag for all items in list
	void setSelected(bool select);

protected:
	int groupId;
	TAKELIST TakeList;
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
	void setSelected(bool v)
	{
		for (auto & item : list)
			item.setSelected(v);
	}

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

/*
convenient audio functions to be used inside an audioprocess function
*/
class AUDIOFUNCTION
{
public:
	static double getPeakValue(TAKE& take, double * frameIndexOut = nullptr, double * channelIndexOut = nullptr);
	static double getPeakValueAbsolute(TAKE & take, double * frameIndexOut = nullptr, double * channelIndexOut = nullptr);

	static vector<double> sumChannelModeChannels(TAKE & take);

	static vector<double> sumAllChannels(TAKE & take);

	static double getPeakRMS(TAKE & take, double timeWindowForPeakRMS);

	static bool isAudioSilent(TAKE & take, double minimumAmplitude);
};

class AUDIOPROCESS
{
public:
	static void processTakeList(TAKELIST& list, function<void(TAKE&)> perTakeFunction);

	static void shorthand(TAKE& take, function<void(int,int)> func)
	{
		for (int fr = 0; fr < take.getNumFrames(); ++fr)
			for (int ch = take.getFirstChannel(); ch < take.getLastChannel(); ++ch)
				func(ch, fr);
	}

	static void prepareToStart();
	static void prepareToEnd();
	static void loadTake(TAKE& take);
	static void unloadTake(TAKE& take);
};
