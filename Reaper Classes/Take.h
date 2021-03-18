#pragma once

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
	MIDINOTE(int pitch, double startTime, double endTime, int velocity, int channel = 1, bool selected = false, bool muted = false)
		: pitch(pitch), startTime(startTime), endTime(endTime), velocity(velocity), channel(channel), selected(selected), muted(muted)
	{ }
  
  virtual ~MIDINOTE(){}

	int getPitch() const { return pitch; };
	String getPitchString() { return MIDI(pitch).getName(); }

	void setPitch(int v);

	void setPosition(double v) override;

	double getLength() const const override { return getEnd() - getStart(); }
	void setLength(double v) override;

	double getStart() const override { return startTime; }
	void setStart(double v) override;

	double getEnd() const override { return endTime; }
	void setEnd(double v) override;

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

	int channel = 1;
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
	void remove(int index);
	void removeAll();

protected:
	TAKE * take = nullptr;
};

class TAKEMARKER
{
public:
	static vector<TAKEMARKER> collect(MediaItem_Take* take)
	{
		vector<TAKEMARKER> list;

		int numMarkers = TAKEMARKER::count(take);
		for (int i = 0; i < numMarkers; ++i)
			list.push_back(TAKEMARKER::get(take, i));

		return std::move(list);
	}

	static void replace(MediaItem_Take* take, vector<TAKEMARKER>& list)
	{
		int i = 0;
		while (DeleteTakeMarker(take, i))
			++i;

		for (auto& m : list)
			TAKEMARKER::add(take, m.position, m.name, m.color);
	}

	static int count(MediaItem_Take* take)
	{
		return GetNumTakeMarkers(take);
	}

	static TAKEMARKER add(MediaItem_Take* take, double position, String name = "", int color = 0)
	{
		int idx = SetTakeMarker(take, -1, (char*)name.toRawUTF8(), &position, &color);
		return { take, idx, position, name, color };
	}

	static TAKEMARKER get(MediaItem_Take* take, int idx)
	{
		char c[256];
		String name;
		double position = 0;
		int color = 0;

		if (isPositiveAndBelow(idx, TAKEMARKER::count(take)))
		{
			position = GetTakeMarker(take, idx, c, 256, &color);
			name = c;
		}
		else
		{
			idx = -1;
			jassertfalse;
		}

		return { take, idx, position, name, color };
	}

	TAKEMARKER(MediaItem_Take* take, int idx, double position, String name = "", int color = 0)
		: take(take), idx(idx), position(position), name(name), color(color)	{}

	~TAKEMARKER() = default;

	// removes based on index, returns true if marker exists
	bool remove();

	int getIndex() { return idx; }

	String getName();
	void setName(const String& v);

	Colour getColor();
	void setColor(const Colour& v);

	double getPosition();
	// may cause index to change
	void setPosition(double v);

	int idx = -1; // changes based on position or existence
	MediaItem_Take* take = nullptr;
	String name;
	double position = 0;
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
	double getLocalFromGlobalTime(double global);

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
