#include "ReaperClassesHeader.h"
#include "Take.h"

TAKE InvalidTake;

vector<MediaItem_Take*> GetTakes(MediaItem* item)
{
    int takes = CountTakes(item);
    vector<MediaItem_Take*> list; list.reserve(takes);
    for (int t = 0; t < takes; ++t) list.push_back(GetTake(item, t));
    return list;
}
void RemoveTake(MediaItem* item, MediaItem_Take* take)
{
    auto actTake = GetActiveTake(item);
    SetActiveTake(take);
    COMMAND(40129); //Delete active take from items
    COMMAND(41348); //Item: Remove all empty take lanes
    if (actTake != take) SetActiveTake(actTake);
}

String TAKE::getObjectName() const { return GetTakeName(take); }

void TAKE::setObjectName(const String & v) { GetSetMediaItemTakeInfo_String(take, "P_NAME", (char*)v.toRawUTF8(), 1); }
double TAKE::getObjectStartPos() const { return ITEM(take).startPos(); }
void TAKE::setObjectStartPos(double v) { ITEM(take).startPos(v); }
double TAKE::getObjectLength() const { return ITEM(take).length(); }
void TAKE::setObjectLength(double v) { ITEM(take).length(v); }
int TAKE::getObjectColor() const { return 0; }
void TAKE::setObjectColor(int v) {}
bool TAKE::objectIsValid() const { return take != nullptr; }

TAKE::TAKE(MediaItem_Take * take) : take(take)
{
    jassert(take != nullptr);
    InitAudio();
    TagManager.WithTags(getObjectName());

    envelope.Volume.setTrackEnvelope(take, "Volume");
    envelope.Pan.setTrackEnvelope(take, "Pan");
    envelope.Mute.setTrackEnvelope(take, "Mute");
    envelope.Pitch.setTrackEnvelope(take, "Pitch");
}

// functions
AUDIO& TAKE::audio() { return m_audio; }
int TAKE::idx() const { return GetMediaItemTakeInfo_Value(take, "IP_TAKENUMBER"); }
MediaItem * TAKE::item() const { return GetMediaItemTake_Item(take); }
MediaTrack * TAKE::track() const { return GetMediaItemTrack(item()); }
int TAKE::chanmode() const { return GetMediaItemTakeInfo_Value(take, "I_CHANMODE"); }
struct chantype { enum { normal, mono, stereo }; };
int TAKE::firstCh() const
{
    int ch = chanmode();

    if (ch >= 67) return ch - 67;
    if (ch >= 3) return ch - 3;
    return 0;
}
int TAKE::lastCh() const
{
    int ch = chanmode();
    int first = firstCh();

    if (ch == 0) return GetMediaItemTake_Source(take)->GetNumChannels() - 1;
    if (ch >= 2 || ch <= 66) return first;
    return first + 1;

}
double TAKE::pitch() const { return GetMediaItemTakeInfo_Value(take, "D_PITCH"); }
bool TAKE::preservepitch() const { return GetMediaItemTakeInfo_Value(take, "B_PPITCH") == 1; }
double TAKE::rate() const { return GetMediaItemTakeInfo_Value(take, "D_PLAYRATE"); }
double TAKE::vol() const { return GetMediaItemTakeInfo_Value(take, "D_VOL"); }
double TAKE::offset() const { return GetMediaItemTakeInfo_Value(take, "D_STARTOFFS"); }
int TAKE::srate() { return m_srate; }
int TAKE::bitdepth() { return m_bitdepth; }
int TAKE::nch() { return m_nch; }
PCM_source * TAKE::pcm_source() const { return m_source; }
size_t TAKE::frames() const { return m_take_frames; }
size_t TAKE::samples() const { return m_take_samples; }
size_t TAKE::file_frames() const { return m_file_frames; }
File TAKE::file() const { return m_file; }

void TAKE::file(const String & file) { SetMediaItemTake_Source(take, PCM_Source_CreateFromFile(file.toRawUTF8())); }
void TAKE::chanmode(int v) { SetMediaItemTakeInfo_Value(take, "I_CHANMODE", v); }
void TAKE::vol(double v) { SetMediaItemTakeInfo_Value(take, "D_VOL", v); }
void TAKE::pitch(double v) { SetMediaItemTakeInfo_Value(take, "D_PITCH", v); }
void TAKE::preservepitch(bool v) { SetMediaItemTakeInfo_Value(take, "B_PPITCH", v); }
void TAKE::rate(double v) { SetMediaItemTakeInfo_Value(take, "D_PLAYRATE", v); }
void TAKE::offset(double v) { SetMediaItemTakeInfo_Value(take, "D_STARTOFFS", v); }
TAKE TAKE::activate() { auto old = GetActiveTake(item()); SetActiveTake(take); return old; }
void TAKE::remove() { take = nullptr; RemoveTake(item(), take); }

TAKE TAKE::move(MediaTrack * track)
{
    // duplicate item
    TAKE old_active_take = activate();
    auto new_item = ITEM(item()).duplicate();
    old_active_take.activate();
    MoveMediaItemToTrack(new_item, track);

    // set active take to the new take for item
    TAKE new_take = GetTake(new_item, idx());
    SetActiveTake(new_take);

    // remove all other takes from new item
    UNSELECT_ITEMS();
    ITEM it;
    it.selected(true);
    it.CollectTakes();
    TAKE actTake = GetActiveTake(it);
    TAKELIST TakeList = it.GetTakes();
    for (int t = 0; t < TakeList.size(); ++t) 
        if (actTake != TakeList[t])
            TakeList[t].remove();
    for (const TAKE & t : TakeList) 
        ITEM(t).selected(true);

    // remove take from old item
    remove();

    // overwrite self
    take = new_take;

    return take;
}

TAKE TAKE::move(MediaItem * new_item)
{
    auto new_take = AddTakeToMediaItem(new_item);

    char* chunk = GetSetObjectState(take, "");
    GetSetObjectState(new_take, chunk);
    FreeHeapPtr(chunk);

    // remove take from old item
    remove();

    // overwrite self
    take = new_take;

    return take;
}

void TAKE::InitAudio(double starttime, double endtime)
{
    m_source = GetMediaItemTake_Source(take);

    m_audio = AUDIO((File)m_source->GetFileName());

    /*Needs to be moved to AUDIO constructor*/
    m_audio.m_srate = m_source->GetSampleRate();
    m_audio.m_samples = m_source->GetLength();
    m_audio.m_bitdepth = m_source->GetBitsPerSample();
    m_audio.m_channels = m_source->GetNumChannels();
    m_audio.m_frames = m_audio.m_samples / m_audio.m_channels;
    /***************************************/

    m_file = m_source->GetFileName();
    m_srate = m_source->GetSampleRate();
    m_file_frames = length() * m_srate;
    m_file_length = m_source->GetLength();
    m_bitdepth = m_source->GetBitsPerSample();
    m_nch = m_source->GetNumChannels();

    if (starttime == -1) starttime = m_audiobuf_starttime = 0;
    if (endtime == -1) { endtime = m_audiobuf_endtime = length(); }

    m_take_frames = (m_audiobuf_endtime - m_audiobuf_starttime) * (double)m_srate;
    m_take_samples = m_take_frames * m_nch;
}

void TAKE::LoadAudio()
{
    int initial_chanmode = chanmode();
    chanmode(0);

    vector<double> buffer(m_take_samples, 0);
    AudioAccessor* accessor = CreateTakeAudioAccessor(take);
    GetAudioAccessorSamples(accessor, m_srate, m_nch, m_audiobuf_starttime, m_take_frames, buffer.data());
    DestroyAudioAccessor(accessor);

    chanmode(initial_chanmode);

    m_audiobuf = InterleavedToMultichannel(buffer.data(), m_nch, m_take_frames);
}

void TAKE::UnloadAudio() { m_audiobuf.clear(); }

TAKELIST::TAKELIST() {}
TAKELIST::TAKELIST(MediaItem * item)
{
    int num_takes = CountTakes(item);
    for (int i = 0; i < num_takes; ++i)
    {
        push_back(GetTake(item, i));
    }
}
