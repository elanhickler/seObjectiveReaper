#pragma once

using juce::File;

class TAKE : public OBJECT_MOVABLE, public OBJECT_NAMABLE, public OBJECT_VALIDATES
{
private:
    // member
    MediaItem_Take* take;
    AUDIO m_audio;    

    // audio property
    PCM_source* m_source = nullptr;
    int m_srate = -1;
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

    String getObjectName() const override;
    void setObjectName(const String & v) override;

    double getObjectStartPos() const override;
    void setObjectStartPos(double v) override;

    double getObjectLength() const override;
    void setObjectLength(double v) override;

    int getObjectColor() const override;
    void setObjectColor(int v) override;

    bool objectIsValid() const override;

public:
    // constructor
    TAKE() { take = nullptr; }
    TAKE(MediaItem_Take* take);

    // conversion
    operator void*() const { return take; }
    operator MediaItem_Take*() const { return take; }

    // operator
    bool operator==(const MediaItem_Take * rhs) const { return take == rhs; }
    bool operator!=(const MediaItem_Take * rhs) const { return take != rhs; }
    bool operator==(const TAKE & rhs) const { return take == rhs.take; }
    bool operator!=(const TAKE & rhs) const { return take != rhs.take; }
    vector<double>& operator[](int i) { return m_audiobuf[i]; }

    struct envelope
    {
        ENVELOPE Volume;
        ENVELOPE Pan;
        ENVELOPE Mute;
        ENVELOPE Pitch;
    } envelope;

    // getter
    AUDIO& audio();
    int idx() const;
    MediaItem* item() const;
    MediaTrack* track() const;
    int chanmode() const;
    int firstCh() const;
    int lastCh() const;
    double pitch() const;
    bool preservepitch() const;
    double rate() const;
    double vol() const;
    double offset() const;
    int srate();
    int bitdepth();
    int nch();
    PCM_source* pcm_source() const;
    size_t frames() const;
    size_t samples() const;
    size_t file_frames() const;
    File file() const;

    // setter
    void file(const String & file);
    void chanmode(int v);
    void vol(double v);
    void pitch(double v);
    void preservepitch(bool v);
    void rate(double v);
    void offset(double v);
    TAKE activate();
    void remove();
    TAKE move(MediaTrack* track);
    TAKE move(MediaItem* new_item);

    // audio functions
    void InitAudio(double starttime = -1, double endtime = -1); // Call this for all desired takes first, then offline all media
    void LoadAudio(); // Before you call this, online just the item currently being processed
    void UnloadAudio(); // After you are done with said audio, unload it to reclaim RAM
};

extern TAKE InvalidTake;

class TAKELIST : public LIST<TAKE>
{
private:
    MediaItem* item;

public:
    TAKELIST();
    TAKELIST(MediaItem* item);
};