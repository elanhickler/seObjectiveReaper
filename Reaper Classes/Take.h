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
	MIDINOTE(int pitch, double startTime, double endTime, int velocity, int channel, bool selected, bool muted)
		: pitch(pitch), startTime(startTime), endTime(endTime), velocity(velocity), channel(channel), selected(selected), muted(muted)
	{ }

	int getPitch() { return pitch; };
	String getPitchString() { return MIDI(pitch).getName(); }

	void setPitch(int v);

	void setPosition(double v) override;

	void setLength(double v) override;

	void setStart(double v) override;

	void setEnd(double v) override;

	double getStart() const override { return startTime; }
	double getEnd() const override { return endTime; }
	double getLength() const override { return endTime - startTime; }
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
public:
	MIDINOTELIST() {};
	MIDINOTELIST(TAKE * take) : take(take) {};

	void collectMidiNotes();

	//position in seconds relative to take
	void add(MIDINOTE midinote);

protected:
	TAKE * take;
};

class TAKE : public OBJECT_MOVABLE, public OBJECT_NAMABLE, public OBJECT_VALIDATES
{
public:

	// constructor
	TAKE() {}
	TAKE(MediaItem_Take* take);

	// conversion
	operator void*() const { return takePtr; }
	operator MediaItem_Take*() const { return takePtr; }

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
	MediaItem_Take * ptr()
	{
		jassert(takePtr != nullptr);
		return takePtr;
	}
	PCM_source * pcm_source() const;

	String getName() const override;
	void setName(const String & v) override;

	// getter
	bool isMidi() const { return TakeIsMIDI(takePtr); }
	bool isAudio() const { return !isMidi(); }
	bool isPitchPreserved() const;
	bool isPhaseInverted() const;

	int idx() const;
	MediaItem * item() const;
	MediaTrack * track() const;
	int chanmode() const;
	int firstCh() const;
	int lastCh() const;
	double getPitch() const;
	double getRate() const;
	double getStartOffset() const;
	double getVolume() const;
	int getSampleRate();
	int getBitDepth();
	int getNumChannels();

	size_t frames() const;
	size_t samples() const;
	size_t file_frames() const;
	File file() const;

	// setter
	void setFile(const String & file);
	void setChannelMode(int v);
	void setVolume(double v);
	void setPitch(double v);
	void setPreservePitch(bool v);
	void setInvertPhase(bool v);
	void setRate(double v);
	void setStartOffset(double v);
	TAKE activate();
	void remove();
	TAKE move(MediaTrack* track);
	TAKE move(MediaItem* new_item);

	/* MIDI FUNCTIONS */
	MIDINOTELIST * getMidiNoteList()
	{
		note_list = MIDINOTELIST(this);
		note_list.collectMidiNotes();
		return &note_list;
	}

	/* AUDIO FUNCTIONS */
	/*
	Example usage code:

	// set items offline/online to prevent RAM overusag,
	// this is optional, but recommended
	SET_ALL_ITEMS_OFFLINE();
	UNSELECT_ITEMS();

	for (auto item : ItemList)
	{
	item.selected(true);
	SET_SELECTED_ITEMS_ONLINE();
	auto take = item.getActiveTake();
	take.loadAudio();

	int channel = 0;
	int sample = 0;
	double value = take.getAudioSample(channel, sample);

	take.unloadAudio();
	SET_SELECTED_ITEMS_OFFLINE();
	item.selected(false);
	}

	SET_ALL_ITEMS_ONLINE();
	*/

	bool audioIsInitialized = false;
	void initAudio(double starttime = -1, double endtime = -1);
	void loadAudio();
	void unloadAudio();

	double getAudioSampleRate() { return audioFile.m_srate; }
	int getAudioNumChannels() { return m_audiobuf.size(); }
	int getTakeNumAudioSamplesPerChannel() { return m_take_frames; }

	vector<vector<double>> & getAudioMultichannel();
	vector<double> & getAudioChannel(int channel);
	double getAudioSample(int channel, int sample);
	double getProjectPositionForAudioSample(int sample)
	{
		double sr = getSampleRate();
		double startTime = getStart();
		return startTime + sample / sr;
	}

	bool isValid() const override;

protected:
	// member
	MediaItem_Take* takePtr = nullptr;
	AudioFile audioFile;

	/* MIDI FUNCTIONS */
	MIDINOTELIST note_list;

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

	double getStart() const override;
	void setStart(double v) override;

	double getLength() const override;
	void setLength(double v) override;

	int getColor() const override;
	void setColor(int v) override;
};

class TAKELIST : public LIST<TAKE>
{
protected:
	MediaItem* item;

public:
	TAKELIST();
	TAKELIST(MediaItem* item);
};
