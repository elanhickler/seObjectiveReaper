#pragma once

/*
* BASE OBJECT:

Simple base objects that directly manipulate the reaper project in an object oriented way
CONSTRUCTOR DOES NOTHING TO PROJECT
STATIC FUNCTIONS: 
	count (return number of objects)
	add (adds object to project) 
	get (gets an object from project)
	collect (returns a vector of all objects)
	replace (clears objects from project and rewrites from list)
REMOVE FUNCTION: object's method, object removes itself from project
NEVER USE CONST


NOMENCLATURE:

cursor = edit cursor, not play cursor and not mouse cursor
mouse = mouse cursor
play cursor = edit cursor when not in play mode or cursor that is moving during play mode



use advanced objects to manipulate mass simple objects

make public the raw simple object list such as takeMarkerList and the Take that owns the takeMarkerList, only have read/write/add

never use const auto in loops

use collect actions to gather objects and info on objects
manipulate the list that is created from collecting objects
remove all said objects, rewrite all objects from scratch

READ -> MANIPULATE -> WRITE
*/

using juce::File;

class TAKE;
class ITEM;

class MIDINOTE : public OBJECT_MOVABLE
{
	friend class MIDINOTELIST;

public:
	MIDINOTE(int pitch, double position, double length)
		: pitch(pitch), startTime(position), endTime(position + length)
	{ }
	MIDINOTE(int pitch, double startTime, double endTime, int velocity, int channel = 0, bool selected = false, bool muted = false)
		: pitch(pitch), startTime(startTime), endTime(endTime), velocity(velocity), channel(channel), selected(selected), muted(muted)
	{ }
  
  virtual ~MIDINOTE(){}

	int getPitch() const { return pitch; };
	String getPitchString() { return MIDI(pitch).getName(); }

	void setPitch(int v);
	void setVelocity(int v);
	int getVelocity() { return velocity; }

	void setPosition(double v) override;

	double getLength() const const override { return getEnd() - getStart(); }
	void setLength(double v) override;

	// Returns time based on item, add item start time to get project start time
	double getStart() const override { return startTime; }
	void setStart(double v) override;

	double getEnd() const override { return endTime; }
	void setEnd(double v) override;

	double getProjectStart() const;
	double getProjectEnd() const;

	bool getIsSelected() const { return selected; }
	bool getIsMuted() const { return muted; }

protected:
	TAKE * take = nullptr;
	int index = -1;

	int pitch = 36;
	double startTime = 0;
	double endTime = 0.25;

	bool selected = false;
	bool muted = false;

	int channel = 0;
	int velocity = 127;
};

class MIDINOTELIST : public LIST<MIDINOTE>
{
	friend class TAKE;

public:
	MIDINOTELIST() {};
	MIDINOTELIST(TAKE & take) : take(&take) {};

	void collect();
	void insert(MIDINOTE midinote);
	void append(MIDINOTELIST list);
	void remove(int index);
	void removeAll();

protected:
	TAKE * take = nullptr;
};

class TAKEMARKER
{
public:

	struct MarkerPreset
	{
		MarkerPreset(String name, Colour color) : name(name), color(color) {}
		String name;
		Colour color;
	};

	static const MarkerPreset atk, rel, pre, leg_s, leg_e, rel_s, rel_e, start, end, loop_s, loop_e;

	static int getColorFromPreset(String name)
	{
		name = name.toLowerCase();

		if (name.isEmpty())
			name == "unnamed";

		if (name == "atk")
			return juceToReaperColor(atk.color);
		if (name == "rel")
			return juceToReaperColor(rel.color);
		if (name == "pre")
			return juceToReaperColor(pre.color);
		if (name == "leg-s" || name == "rel-s")
			return juceToReaperColor(leg_s.color);
		if (name == "leg-e" || name == "rel-e")
			return juceToReaperColor(leg_e.color);
		if (name == "start")
			return juceToReaperColor(start.color);
		if (name == "end")
			return juceToReaperColor(end.color);
		if (name == "[")
			return juceToReaperColor(loop_s.color);
		if (name == "]")
			return juceToReaperColor(loop_e.color);

		return 0;
	}

	static vector<TAKEMARKER> collect(TAKE& take, bool ignoreMarkersOutsideItem = true);

	static void replace(TAKE& take, vector<TAKEMARKER>& list);

	static int count(TAKE& take);

	// local time
	static TAKEMARKER add(TAKE& take, double position, String name = "", int color = 0);

	// local time
	static TAKEMARKER addOrUpdateByName(TAKE& take, double position, String name = "", int color = 0);

	// local time
	static TAKEMARKER addOrUpdateByIdx(TAKE& take, int idx, double position, String name = "", int color = 0);

	static TAKEMARKER getByName(TAKE& take, String name, bool ignoreMarkersOutsideItem = true);

	static TAKEMARKER getByIdx(TAKE& take, int idx);

	// local time
	TAKEMARKER(TAKE& take, int idx = -1, double position = 0, String name = "", int color = 0)
		: take(&take), idx(idx), position(position), name(name), color(color)	{ }

	TAKEMARKER() {}

	~TAKEMARKER() = default;

	// removes based on index, returns true if marker exists
	bool remove();

	int getIndex() { return idx; }

	// returns true if marker did exists in project
	bool isValid() { return take != nullptr && idx >= 0; }

	// returns true if take marker is outside the range of the item's length and thus not visible
	bool isOutsideItem();

	String getName();
	void setName(const String& v);

	Colour getColor();
	void setColor(const Colour& v);

	// local to take
	double getPosition();
	double getPositionInItem();
	// may cause index to change
	void setPosition(double v);

	int idx = -1; // changes based on position or existence
	TAKE* take;
	String name;
	double position = 0; // local time
	int color = 0; // default color is 0 based on theme	
};

class TAKE : public OBJECT_MOVABLE, public OBJECT_NAMABLE, public OBJECT_VALIDATES
{
	friend class AUDIOPROCESS;
	friend class ITEM;
	friend class MIDINOTE;
	friend class MIDINOTELIST;

public:
	TAKE() {}
	TAKE(MediaItem_Take* take);
	TAKE(MediaItem* item);
	TAKE(const vector<vector<double>> & multichannelAudio, FILE fileToWriteTo);

	// conversion
	//operator void*() const { return takePtr; }
	//operator MediaItem_Take*() const { return takePtr; }

	// operator
	bool operator==(const MediaItem_Take * rhs) const { return takePtr == rhs; }
	bool operator!=(const MediaItem_Take * rhs) const { return takePtr != rhs; }
	bool operator==(const TAKE & rhs) const { return takePtr == rhs.takePtr; }
	bool operator!=(const TAKE & rhs) const { return takePtr != rhs.takePtr; }
	vector<double>& operator[](int i) { return takeAudioBuffer[i]; }

	struct envelope
	{
		ENVELOPE Volume;
		ENVELOPE Pan;
		ENVELOPE Mute;
		ENVELOPE Pitch;
	} envelope;

	MediaItem_Take * getPointer() const { return takePtr; }
	MediaItem_Take * getPointer() { return takePtr; }
	PCM_source * getPCMSource() const;

	// getter
	bool isMidi() const { return TakeIsMIDI(takePtr); }
	bool isAudio() const { return !isMidi(); }
	bool isPitchPreserved() const;
	bool isPhaseInverted() const;

	int getIndex() const;
	MediaItem * getMediaItemPtr() const;
	MediaTrack * track() const;
	int getChannelMode() const;
	int getFirstChannel() const;
	int getLastChannel() const;
	double getPitch() const;
	double getRate() const;
	double getStartOffset() const;
	double getVolume() const;
	File getFile() const;

	// setter
	void setFile(const File & file);
	void setChannelMode(int v);
	void setVolume(double v);
	void setPitch(double v);
	void setPreservePitch(bool v);
	void setInvertPhase(bool v);
	void setRate(double v);
	void setStartOffset(double v);
	void activate();
	void remove();

	double getStart() const override;
	void setStart(double v) override;

	double getEnd() const override;
	void setEnd(double v) override {}

	double getLength() const override;
	void setLength(double v) override;

	Colour getColor() const override;
	void setColor(Colour v) override;

	// uses item rate and take offset to calculate actual take position
	double globalToLocalTime(double global);
	double localToGlobalTime(double local);

	TAKE move(MediaTrack* track);
	TAKE move(MediaItem* new_item);

	int countStretchMarkers() const
	{
		return GetTakeNumStretchMarkers(takePtr);
	}
	void addStretchMarker(double position)
	{
		SetTakeStretchMarker(takePtr, -1, position, nullptr);
	}
	void clearStretchMarkers()
	{
		int total = countStretchMarkers();
		DeleteTakeStretchMarkers(takePtr, 0, &total);
	}

	void initAudio(double starttime = -1, double endtime = -1);
	void loadAudio();
	void unloadAudio();
	bool isAudioInitialized();

	AUDIODATA& getAudioFile();
	AUDIODATA& getTakeAudio() { return takeAudioBuffer; }
	int getSampleRate();
	int getBitDepth();
	// return total channels for source audio
	int getNumChannels();
	// return number of active channels based on channel mode
	int getNumChannelModeChannels();
	size_t getNumFrames() const;
	size_t getNumSamples() const;

	vector<vector<double>> & getAudioMultichannel();
	vector<double> & getAudioChannel(int channel);
	double getSample(int channel, int frame);
	double getProjectPositionForFrameIndex(int index);

	bool isValid() const override;

protected:
	// member
	MediaItem_Take* takePtr = nullptr;
	ITEM * itemParent = nullptr;
	AUDIODATA audioFile;
	bool audioIsInitialized = false;

	AUDIODATA takeAudioBuffer;
	size_t takeFrames = 0;
	size_t takeSamples = 0;
	double audiobuf_starttime = -1;
	double audiobuf_endtime = -1;

	String getObjectName() const override;
	void setObjectName(const String & v) override;
};


class TAKELIST : public LIST<TAKE>
{
public:
	static TAKELIST getSelectedActiveTakes()
	{
		TAKELIST takelist;
		takelist.collectSelectedActiveTakes();
		return std::move(takelist);
	}

	TAKELIST() {}

	void collectSelectedActiveTakes()
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
};
