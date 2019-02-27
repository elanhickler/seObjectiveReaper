#pragma once

#include "../Elan Classes/MidiHelper.h"

using juce::File;

class TAKE;

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

	int getPitch() { return pitch; };
	String getPitchString() { return MIDI(pitch).getName(); }

	void setPitch(int v);

	void setPosition(double v) override;

	double getLength() const override { return getEnd() - getStart(); }
	void setLength(double v) override;

	double getStart() const override { return startTime; }
	void setStart(double v) override;

	double getEnd() const override { return endTime; }
	void setEnd(double v) override;

	bool getIsSelected() { return selected; }
	bool getIsMuted() { return muted; }

protected:
	TAKE * take;
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

class TAKE : public OBJECT_MOVABLE, public OBJECT_NAMABLE, public OBJECT_VALIDATES
{
	friend class AUDIOPROCESS;
	friend class ITEM;
	friend class MIDINOTE;
	friend class MIDINOTELIST;

public:
	TAKE() {}
	TAKE(MediaItem_Take* take);

	// conversion
	//operator void*() const { return takePtr; }
	//operator MediaItem_Take*() const { return takePtr; }

	// operator
	bool operator==(const MediaItem_Take * rhs) const { return takePtr == rhs; }
	bool operator!=(const MediaItem_Take * rhs) const { return takePtr != rhs; }
	bool operator==(const TAKE & rhs) const { return takePtr == rhs.takePtr; }
	bool operator!=(const TAKE & rhs) const { return takePtr != rhs.takePtr; }
	vector<double>& operator[](int i) { return m_audiobuf[i]; }

	struct envelope
	{
		ENVELOPE Volume;
		ENVELOPE Pan;
		ENVELOPE Mute;
		ENVELOPE Pitch;
	} envelope;

	AudioFile & audio();
	MediaItem_Take * getPointer() const { return takePtr; }
	MediaItem_Take * getPointer() { return takePtr; }
	PCM_source * pcm_source() const;

	String getName() const override;
	void setName(const String & v) override;

	// getter
	bool isMidi() const { return TakeIsMIDI(takePtr); }
	bool isAudio() const { return !isMidi(); }
	bool isPitchPreserved() const;
	bool isPhaseInverted() const;

	int getIndex() const;
	MediaItem * getMediaItemPtr() const;
	MediaTrack * track() const;
	int chanmode() const;
	int getFirstChannel() const;
	int getLastChannel() const;
	double getPitch() const;
	double getRate() const;
	double getStartOffset() const;
	double getVolume() const;
	int getSampleRate();
	int getBitDepth();
	int getNumChannels();

	size_t getNumFrames() const;
	size_t getNumSamples() const;
	File getFile() const;

	// setter
	void setFile(const String & file);
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

	bool audioIsInitialized = false;
	void initAudio(double starttime = -1, double endtime = -1);
	void loadAudio();
	void unloadAudio();

	double getAudioSampleRate() { return audioFile.srate; }
	int getAudioNumChannels() { return m_audiobuf.size(); }
	int getAudioNumFrames() { return m_take_frames; }

	vector<vector<double>> & getAudioMultichannel();
	vector<double> & getAudioChannel(int channel);
	double getAudioSample(int channel, int sample);
	double getProjectPositionForFrameIndex(int sample)
	{
		double sr = getSampleRate();
		double startTime = getStart();
		return startTime + sample / sr;
	}

	bool isValid() const override;

protected:
	// member
	MediaItem_Take* takePtr = nullptr;
	ITEM * itemParent = nullptr;
	AudioFile audioFile;

	// audio property
	PCM_source* m_source = nullptr;
	int m_nch = -1;
	int m_bitdepth = -1;

	File m_file;
	int m_file_frames = -1;
	double m_file_length = -1.0;

	vector<vector<double>> m_audiobuf;
	size_t m_take_frames = 0;
	size_t m_take_samples = 0;
	double m_audio_starttime = -1;
	double m_audio_endttime = -1;
	double m_audiobuf_starttime = -1;
	double m_audiobuf_endtime = -1;
};
