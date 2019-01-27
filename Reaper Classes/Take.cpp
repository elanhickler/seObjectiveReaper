#include "ReaperClassesHeader.h"
#include "Take.h"

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

String TAKE::getName() const { return GetTakeName(takePtr); }

void TAKE::setName(const String & v) { GetSetMediaItemTakeInfo_String(takePtr, "P_NAME", (char*)v.toRawUTF8(), 1); }
double TAKE::getStart() const { return ITEM(takePtr).getStart(); }
void TAKE::setStart(double v) { ITEM(takePtr).setStart(v); }
double TAKE::getLength() const { return ITEM(takePtr).getLength(); }
void TAKE::setLength(double v) { ITEM(takePtr).setLength(v); }
int TAKE::getColor() const { return 0; }
void TAKE::setColor(int v) {}
bool TAKE::isValid() const { return takePtr != nullptr; }

TAKE::TAKE(MediaItem_Take * take) : takePtr(take)
{
    jassert(take != nullptr);
    initAudio();
    TagManager.setStringWithTags(getName());

    envelope.Volume.setTrackEnvelope(take, "Volume");
    envelope.Pan.setTrackEnvelope(take, "Pan");
    envelope.Mute.setTrackEnvelope(take, "Mute");
    envelope.Pitch.setTrackEnvelope(take, "Pitch");
}

// functions
AudioFile & TAKE::audio() { return audioFile; }
int TAKE::idx() const { return GetMediaItemTakeInfo_Value(takePtr, "IP_TAKENUMBER"); }
MediaItem * TAKE::item() const { return GetMediaItemTake_Item(takePtr); }
MediaTrack * TAKE::track() const { return GetMediaItemTrack(item()); }
int TAKE::chanmode() const { return GetMediaItemTakeInfo_Value(takePtr, "I_CHANMODE"); }
struct chantype { enum { normal, mono, stereo }; };
int TAKE::firstCh() const
{
    int ch = chanmode();

    if (ch >= 67)
      return ch - 67;
    if (ch >= 3)
      return ch - 3;
    return 0;
}
int TAKE::lastCh() const
{
    int ch = chanmode();
    int first = firstCh();

    if (ch == 0) 
      return GetMediaItemTake_Source(takePtr)->GetNumChannels() - 1;
    if (ch >= 2 || ch <= 66) 
      return first;
    return first + 1;
}

bool TAKE::isPitchPreserved() const { return GetMediaItemTakeInfo_Value(takePtr, "B_PPITCH") != 0; }
bool TAKE::isPhaseInverted() const { return GetMediaItemTakeInfo_Value(takePtr, "D_VOL") < 0; }

double TAKE::getPitch() const { return GetMediaItemTakeInfo_Value(takePtr, "D_PITCH"); }
double TAKE::getRate() const { return GetMediaItemTakeInfo_Value(takePtr, "D_PLAYRATE"); }

// Returns volume as a factor of amplitude.
double TAKE::getVolume() const { return abs(GetMediaItemTakeInfo_Value(takePtr, "D_VOL")); }
double TAKE::getStartOffset() const { return GetMediaItemTakeInfo_Value(takePtr, "D_STARTOFFS"); }
int TAKE::getSampleRate() { return audioFile.m_srate; }
int TAKE::getBitDepth() { return m_bitdepth; }
int TAKE::getNumChannels() { return m_nch; }
PCM_source * TAKE::pcm_source() const { return m_source; }
size_t TAKE::frames() const { return m_take_frames; }
size_t TAKE::samples() const { return m_take_samples; }
size_t TAKE::file_frames() const { return m_file_frames; }
File TAKE::file() const { return m_file; }
void TAKE::setFile(const String & file) { SetMediaItemTake_Source(takePtr, PCM_Source_CreateFromFile(file.toRawUTF8())); }
void TAKE::setChannelMode(int v) { SetMediaItemTakeInfo_Value(takePtr, "I_CHANMODE", v); }

void TAKE::setVolume(double v) 
{ 
  bool phaseIsInverted = GetMediaItemTakeInfo_Value(takePtr, "D_VOL") < 0;

  if (phaseIsInverted)
    v = -abs(v);
  else
    v = abs(v);

  SetMediaItemTakeInfo_Value(takePtr, "D_VOL", v); 
}
void TAKE::setPitch(double v) { SetMediaItemTakeInfo_Value(takePtr, "D_PITCH", v); }
void TAKE::setPreservePitch(bool v) { SetMediaItemTakeInfo_Value(takePtr, "B_PPITCH", v); }

void TAKE::setInvertPhase(bool v) 
{
  bool phaseIsInverted = GetMediaItemTakeInfo_Value(takePtr, "D_VOL") < 0;

  if (v != phaseIsInverted)
  { 
    SetMediaItemTakeInfo_Value(takePtr, "D_VOL", -GetMediaItemTakeInfo_Value(takePtr, "D_VOL"));
  }
}

void TAKE::setRate(double v) { SetMediaItemTakeInfo_Value(takePtr, "D_PLAYRATE", v); }
void TAKE::setStartOffset(double v) { SetMediaItemTakeInfo_Value(takePtr, "D_STARTOFFS", v); }
TAKE TAKE::activate() { auto old = GetActiveTake(item()); SetActiveTake(takePtr); return old; }
void TAKE::remove() { takePtr = nullptr; RemoveTake(item(), takePtr); }

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
    it.setSelected(true);
    it.CollectTakes();
    TAKE actTake = GetActiveTake(it);
    TAKELIST TakeList = it.GetTakes();
    for (int t = 0; t < TakeList.size(); ++t) 
        if (actTake != TakeList[t])
            TakeList[t].remove();
    for (const TAKE & t : TakeList) 
        ITEM(t).setSelected(true);

    // remove take from old item
    remove();

    // overwrite self
    takePtr = new_take;

    return takePtr;
}

TAKE TAKE::move(MediaItem * new_item)
{
    auto new_take = AddTakeToMediaItem(new_item);

    char* chunk = GetSetObjectState(takePtr, "");
    GetSetObjectState(new_take, chunk);
    FreeHeapPtr(chunk);

    // remove take from old item
    remove();

    // overwrite self
    takePtr = new_take;

    return takePtr;
}

void TAKE::initAudio(double starttime, double endtime)
{
  audioIsInitialized = true;
    m_source = GetMediaItemTake_Source(takePtr);

    audioFile = AudioFile((File)m_source->GetFileName());

    // raw audio file properties
    audioFile.m_srate = m_source->GetSampleRate();
    audioFile.m_samples = m_source->GetLength();
    audioFile.m_bitdepth = m_source->GetBitsPerSample();
    audioFile.m_channels = m_source->GetNumChannels();
    if (audioFile.m_channels > 0) // occurs if audio file is bad
      audioFile.m_frames = audioFile.m_samples / audioFile.m_channels;
    
    // take audio properties
    audioFile.m_file = m_source->GetFileName();
    m_file_frames = getLength() * audioFile.m_srate;
    m_file_length = m_source->GetLength();
    m_bitdepth = m_source->GetBitsPerSample();
    m_nch = m_source->GetNumChannels();

    if (starttime == -1) starttime = m_audiobuf_starttime = 0;
    if (endtime == -1) { endtime = m_audiobuf_endtime = getLength(); }

    m_take_frames = (m_audiobuf_endtime - m_audiobuf_starttime) * (double)audioFile.m_srate;
    m_take_samples = m_take_frames * m_nch;
}

void TAKE::loadAudio()
{
    int initial_chanmode = chanmode();
    setChannelMode(0);

    vector<double> buffer(m_take_samples, 0);
    AudioAccessor* accessor = CreateTakeAudioAccessor(takePtr);
    GetAudioAccessorSamples(accessor, audioFile.m_srate, m_nch, m_audiobuf_starttime, m_take_frames, buffer.data());
    DestroyAudioAccessor(accessor);

    setChannelMode(initial_chanmode);

    m_audiobuf = InterleavedToMultichannel(buffer.data(), m_nch, m_take_frames);
}

void TAKE::unloadAudio() { m_audiobuf.clear(); }

vector<vector<double>> & TAKE::getAudioMultichannel() { return m_audiobuf; }

vector<double>& TAKE::getAudioChannel(int channel)
{
  jassert(channel < m_audiobuf.size());
  return m_audiobuf[channel];
}

double TAKE::getAudioSample(int channel, int sample) 
{
  jassert(channel < m_audiobuf.size());
  jassert(sample < m_audiobuf[channel].size());
  return m_audiobuf[channel][sample];
}

TAKELIST::TAKELIST() {}
TAKELIST::TAKELIST(MediaItem * item)
{
    int num_takes = CountTakes(item);
    for (int i = 0; i < num_takes; ++i)
    {
        push_back(GetTake(item, i));
    }
}

void MIDINOTELIST::collectMidiNotes()
{
  LIST::clear();

  int notecount;
  MIDI_CountEvts(take->ptr(), &notecount, nullptr, nullptr);
  for (int n = 0; n < notecount; ++n)
  {
    double ppqStart, ppqEnd;
    int note, vel, channel;
    bool selected, muted;

    MIDI_GetNote(take->ptr(), n, &selected, &muted, &ppqStart, &ppqEnd, &channel, &note, &vel);

    double startTime = MIDI_GetProjTimeFromPPQPos(*take, ppqStart) - ITEM(*take).getStart();
    double endTime = MIDI_GetProjTimeFromPPQPos(*take, ppqEnd) - ITEM(*take).getStart();

    push_back({ note, startTime, endTime, vel, channel, muted, selected });
  }
}

void MIDINOTELIST::add(MIDINOTE obj)
{
  obj.take = take;
  double position = obj.getStart() + ITEM(*take).getStart();

  double ppq_start = MIDI_GetPPQPosFromProjTime(take->ptr(), position);
  double ppq_end = MIDI_GetPPQPosFromProjTime(take->ptr(), position + obj.getLength());

  bool noSort = false;
  MIDI_InsertNote(take->ptr(), obj.selected, obj.muted, ppq_start, ppq_end, obj.channel, obj.pitch, obj.velocity, &noSort);
}

void MIDINOTE::setPitch(int v)
{
	bool noSort = false;
	MIDI_SetNote(take->ptr(), index, nullptr, nullptr, nullptr, nullptr, nullptr, &v, nullptr, &noSort);

	pitch = v;
}

void MIDINOTE::setPosition(double v)
{
	double position = v + ITEM(*take).getStart();
	double ppq_start = MIDI_GetPPQPosFromProjTime(take->ptr(), position);

	double ppq_start_original;
	double ppq_end_original;
	MIDI_GetNote(take->ptr(), index, nullptr, nullptr, &ppq_start_original, &ppq_end_original, nullptr, nullptr, nullptr);
	double ppq_end = ppq_start + (ppq_end_original - ppq_start_original);

	bool noSort = false;
	MIDI_SetNote(take->ptr(), index, nullptr, nullptr, &ppq_start, &ppq_end, nullptr, nullptr, nullptr, &noSort);

	startTime = v;
}

void MIDINOTE::setLength(double v)
{
	endTime = startTime + v;
}

void MIDINOTE::setStart(double v)
{
	double start = v + ITEM(*take).getStart();
	double ppq_start = MIDI_GetPPQPosFromProjTime(take->ptr(), start);

	double ppq_end_original;
	MIDI_GetNote(take->ptr(), index, nullptr, nullptr, nullptr, &ppq_end_original, nullptr, nullptr, nullptr);

	bool noSort = false;
	MIDI_SetNote(take->ptr(), index, nullptr, nullptr, &ppq_start, &ppq_end_original, nullptr, nullptr, nullptr, &noSort);

	startTime = v;
}

inline void MIDINOTE::setEnd(double v)
{
	double end = v + ITEM(*take).getStart();
	double ppq_end = MIDI_GetPPQPosFromProjTime(take->ptr(), end);

	double ppq_start_original;
	MIDI_GetNote(take->ptr(), index, nullptr, nullptr, &ppq_start_original, nullptr, nullptr, nullptr, nullptr);

	bool noSort = false;
	MIDI_SetNote(take->ptr(), index, nullptr, nullptr, &ppq_start_original, &ppq_end, nullptr, nullptr, nullptr, &noSort);

	endTime = v;
}
