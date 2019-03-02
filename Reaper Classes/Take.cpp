#include "ReaperClassesHeader.h"
#include "Take.h"





String TAKE::getObjectName() const { return GetTakeName(takePtr); }
void TAKE::setObjectName(const String & v) { GetSetMediaItemTakeInfo_String(takePtr, "P_NAME", (char*)v.toRawUTF8(), 1); }
double TAKE::getStart() const { return GetMediaItemInfo_Value(GetMediaItemTake_Item(takePtr), "D_POSITION"); }
double TAKE::getEnd() const { return getStart() + getLength(); }
void TAKE::setStart(double v) { itemParent->setStart(v); }
double TAKE::getLength() const { return GetMediaItemInfo_Value(GetMediaItemTake_Item(takePtr), "D_LENGTH"); }
void TAKE::setLength(double v) { SetMediaItemInfo_Value(GetMediaItemTake_Item(takePtr), "D_LENGTH", v); }
Colour TAKE::getColor() const
{
	return reaperToJuceColor(GetMediaItemTakeInfo_Value(takePtr, "I_CUSTOMCOLOR"));
}
void TAKE::setColor(Colour v)
{
	SetMediaItemTakeInfo_Value(takePtr, "I_CUSTOMCOLOR", juceToReaperColor(v));
}
bool TAKE::isValid() const { return takePtr != nullptr; }

TAKE::TAKE(MediaItem_Take * take) : takePtr(take)
{
	jassert(take != nullptr);

	OBJECT_NAMABLE::initialize();

	envelope.Volume = ENVELOPE(take, "Volume");
	envelope.Pan = ENVELOPE(take, "Pan");
	envelope.Mute = ENVELOPE(take, "Mute");
	envelope.Pitch = ENVELOPE(take, "Pitch");
}

TAKE::TAKE(MediaItem * item) : takePtr(GetActiveTake(item))
{
	OBJECT_NAMABLE::initialize();
}

// functions
AUDIODATA & TAKE::getAudioFile() { return audioFile; }
int TAKE::getIndex() const { return GetMediaItemTakeInfo_Value(takePtr, "IP_TAKENUMBER"); }
MediaItem * TAKE::getMediaItemPtr() const { return GetMediaItemTake_Item(takePtr); }
MediaTrack * TAKE::track() const { return GetMediaItemTrack(getMediaItemPtr()); }
int TAKE::getChannelMode() const { return GetMediaItemTakeInfo_Value(takePtr, "I_CHANMODE"); }
struct chantype { enum { normal, mono, stereo }; };
int TAKE::getFirstChannel() const
{
	int ch = getChannelMode();

	if (ch >= 67)
		return ch - 67;
	if (ch >= 3)
		return ch - 3;
	return 0;
}
int TAKE::getLastChannel() const
{
	int ch = getChannelMode();
	int first = getFirstChannel();

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
PCM_source * TAKE::pcm_source() const { return pcmSource; }
File TAKE::getFile() const { return audioFile.getFile(); }
void TAKE::setFile(const File & file)
{
	pcmSource = PCM_Source_CreateFromFile(file.getFullPathName().toRawUTF8());
	SetMediaItemTake_Source(takePtr, pcmSource);

	audioFile.clear();
	audioIsInitialized = false;
	takeAudioBuffer.clear();
	takeFrames = 0;
	takeSamples = 0;
	audiobuf_starttime = -1;
	audiobuf_endtime = -1;
}
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
void TAKE::activate() { itemParent->setActiveTake(*this); }
void TAKE::remove()
{
	PROJECT::saveItemSelection();
	PROJECT::unselectAllItems();

	PROJECT::selectItem(itemParent->getPointer());

	auto actTake = GetActiveTake(itemParent->getPointer());
	SetActiveTake(takePtr);
	COMMAND(40129); // Delete active take from selected items
	COMMAND(41348); // Remove all empty take lanes from selected items
	if (actTake != takePtr)
		SetActiveTake(actTake);

	takePtr = nullptr;

	PROJECT::loadItemSelection();
}

TAKE TAKE::move(MediaTrack * track)
{
	//// duplicate item
	//TAKE old_active_take = activate();
	//auto new_item = itemParent->duplicate();
	//old_active_take.activate();
	//MoveMediaItemToTrack(new_item, track);

	//// set active take to the new take for item
	//TAKE new_take = GetTake(new_item, idx());
	//SetActiveTake(new_take);

	//// remove all other takes from new item
	//UNSELECT_ITEMS();
	//ITEM it;
	//it.setSelected(true);
	//TAKE actTake = GetActiveTake(it);
	//for (int t = 0; t < it.size(); ++t)
	//	if (actTake != it[t])
	//		it[t].remove();
	//for (const TAKE & t : it)
	//	ITEM(t).setSelected(true);

	//// remove take from old item
	//remove();

	//// overwrite self
	//takePtr = new_take;

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

/* MIDI FUNCTIONS */

void TAKE::initAudio(double starttime, double endtime)
{
	audioIsInitialized = true;

	audioFile.setSource(pcmSource);

	jassert(audioFile.getNumChannels() > 0); // bad audio file

	if (starttime == -1)
		starttime = audiobuf_starttime = 0;
	if (endtime == -1)
		endtime = audiobuf_endtime = getLength();

	takeFrames = (audiobuf_endtime - audiobuf_starttime) * (double)audioFile.getSampleRate();
	takeSamples = takeFrames * audioFile.getNumChannels();
}

void TAKE::loadAudio()
{
	int initial_chanmode = getChannelMode();
	setChannelMode(0);

	vector<double> buffer(takeSamples, 0);
	AudioAccessor* accessor = CreateTakeAudioAccessor(takePtr);
	GetAudioAccessorSamples(accessor, audioFile.getSampleRate(), audioFile.getNumChannels(), audiobuf_starttime, takeFrames, buffer.data());
	DestroyAudioAccessor(accessor);

	setChannelMode(initial_chanmode);

	takeAudioBuffer = InterleavedToMultichannel(buffer.data(), audioFile.getNumChannels(), takeFrames);
}

void TAKE::unloadAudio() { takeAudioBuffer.clear(); }

int TAKE::getSampleRate() { return audioFile.getSampleRate(); }

int TAKE::getBitDepth() { return audioFile.getBitDepth(); }

int TAKE::getNumChannels() { return audioFile.getNumChannels(); }

size_t TAKE::getNumFrames() const { return takeFrames; }

size_t TAKE::getNumSamples() const { return takeSamples; }

vector<vector<double>> & TAKE::getAudioMultichannel() { return takeAudioBuffer; }

vector<double>& TAKE::getAudioChannel(int channel)
{
	return takeAudioBuffer[channel];
}

double TAKE::getAudioSample(int channel, int frame)
{
	return takeAudioBuffer[channel][frame];
}

void MIDINOTELIST::collect()
{
	LIST::clear();

	int notecount;
	MIDI_CountEvts(take->getPointer(), &notecount, nullptr, nullptr);
	for (int n = 0; n < notecount; ++n)
	{
		double ppqStart, ppqEnd;
		int note, vel, channel;
		bool selected, muted;

		MIDI_GetNote(take->getPointer(), n, &selected, &muted, &ppqStart, &ppqEnd, &channel, &note, &vel);

		double startTime = MIDI_GetProjTimeFromPPQPos(take->getPointer(), ppqStart) - take->getStart();
		double endTime = MIDI_GetProjTimeFromPPQPos(take->getPointer(), ppqEnd) - take->getStart();

		push_back({ note, startTime, endTime, vel, channel, muted, selected });
		back().take = take;
		back().index = n;
	}
}

void MIDINOTELIST::insert(MIDINOTE obj)
{
	//obj.take = take;
	double position = obj.getStart() + take->getStart();

	double ppq_start = MIDI_GetPPQPosFromProjTime(take->getPointer(), position);
	double ppq_end = MIDI_GetPPQPosFromProjTime(take->getPointer(), position + obj.getLength());

	bool noSort = false;
	MIDI_InsertNote(take->getPointer(), obj.selected, obj.muted, ppq_start, ppq_end, obj.channel, obj.pitch, obj.velocity, &noSort);
}

inline void MIDINOTELIST::remove(int index)
{
	MIDI_DeleteNote(take->getPointer(), index);
}

inline void MIDINOTELIST::removeAll()
{
	int notecount;
	MIDI_CountEvts(take->getPointer(), &notecount, nullptr, nullptr);
	for (int i = 0; i < notecount; ++i)
		MIDI_DeleteNote(take->getPointer(), i);
}

void MIDINOTE::setPitch(int v)
{
	bool noSort = false;
	MIDI_SetNote(take->getPointer(), index, nullptr, nullptr, nullptr, nullptr, nullptr, &v, nullptr, &noSort);

	pitch = v;
}

void MIDINOTE::setPosition(double v)
{
	double position = v + take->getStart();
	double ppq_start = MIDI_GetPPQPosFromProjTime(take->getPointer(), position);

	double ppq_start_original;
	double ppq_end_original;
	MIDI_GetNote(take->getPointer(), index, nullptr, nullptr, &ppq_start_original, &ppq_end_original, nullptr, nullptr, nullptr);
	double ppq_end = ppq_start + (ppq_end_original - ppq_start_original);

	bool noSort = false;
	MIDI_SetNote(take->getPointer(), index, nullptr, nullptr, &ppq_start, &ppq_end, nullptr, nullptr, nullptr, &noSort);

	startTime = v;
}

void MIDINOTE::setLength(double v)
{
	endTime = startTime + v;
}

void MIDINOTE::setStart(double v)
{
	double start = v + take->getStart();
	double ppq_start = MIDI_GetPPQPosFromProjTime(take->getPointer(), start);

	double ppq_end_original;
	MIDI_GetNote(take->getPointer(), index, nullptr, nullptr, nullptr, &ppq_end_original, nullptr, nullptr, nullptr);

	bool noSort = false;
	MIDI_SetNote(take->getPointer(), index, nullptr, nullptr, &ppq_start, &ppq_end_original, nullptr, nullptr, nullptr, &noSort);

	startTime = v;
}

void MIDINOTE::setEnd(double v)
{
	double end = v + take->getStart();
	double ppq_end = MIDI_GetPPQPosFromProjTime(take->getPointer(), end);

	double ppq_start_original;
	MIDI_GetNote(take->getPointer(), index, nullptr, nullptr, &ppq_start_original, nullptr, nullptr, nullptr, nullptr);

	bool noSort = false;
	MIDI_SetNote(take->getPointer(), index, nullptr, nullptr, &ppq_start_original, &ppq_end, nullptr, nullptr, nullptr, &noSort);

	endTime = v;
}
