#include "ReaperClassesHeader.h"
#include "Take.h"

const TAKEMARKER::MarkerPreset TAKEMARKER::atk = TAKEMARKER::MarkerPreset{ "atk", Colour::fromRGB(149, 0, 0) };
const TAKEMARKER::MarkerPreset TAKEMARKER::rel = TAKEMARKER::MarkerPreset{ "rel", Colour::fromRGB(47,94,255) };
const TAKEMARKER::MarkerPreset TAKEMARKER::pre = TAKEMARKER::MarkerPreset{ "pre", Colour::fromRGB(67,156,81) };
const TAKEMARKER::MarkerPreset TAKEMARKER::leg_s = TAKEMARKER::MarkerPreset{ "leg-s", Colour::fromRGB(47,94,255) };
const TAKEMARKER::MarkerPreset TAKEMARKER::leg_e = TAKEMARKER::MarkerPreset{ "leg-e", Colour::fromRGB(149,0,0) };
const TAKEMARKER::MarkerPreset TAKEMARKER::rel_s = TAKEMARKER::MarkerPreset{ "leg-s", Colour::fromRGB(47,94,255) };
const TAKEMARKER::MarkerPreset TAKEMARKER::rel_e = TAKEMARKER::MarkerPreset{ "leg-e", Colour::fromRGB(149,0,0) };
const TAKEMARKER::MarkerPreset TAKEMARKER::start = TAKEMARKER::MarkerPreset{ "start", Colour::fromRGB(82,82,82) };
const TAKEMARKER::MarkerPreset TAKEMARKER::end = TAKEMARKER::MarkerPreset{ "end", Colour::fromRGB(82,82,82) };


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

	initAudio();
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
PCM_source * TAKE::getPCMSource() const
{
	return GetMediaItemTake_Source(takePtr);
}
File TAKE::getFile() const
{
	char buf[1024];
	GetMediaSourceFileName(getPCMSource(), buf, 1024);
	return File(buf);
}
void TAKE::setFile(const File & file)
{
	SetMediaItemTake_Source(takePtr, PCM_Source_CreateFromFile(file.getFullPathName().toRawUTF8()));

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


// uses item rate and take offset to calculate actual take position

double TAKE::globalToLocalTime(double global)
{
	auto item = ITEM(getMediaItemPtr());
	return (global - item.getStart()) * item.getRate() + getStartOffset();
}

double TAKE::localToGlobalTime(double local)
{
	auto item = ITEM(getMediaItemPtr());
	return (local - getStartOffset()) / item.getRate() + item.getStart();
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
	audioFile.setSource(getPCMSource());

	if (audioFile.getLength() <= 0 || // audio file offline
		audioFile.getNumChannels() <= 0)  // bad audio file
	{		
		jassertfalse;
		return;
	}

	audioIsInitialized = true;

	if (starttime == -1)
		starttime = audiobuf_starttime = 0;
	if (endtime == -1)
		endtime = audiobuf_endtime = getLength();

	takeFrames = (audiobuf_endtime - audiobuf_starttime) * (double)audioFile.getSampleRate();
	takeSamples = takeFrames * audioFile.getNumChannels();
}

void TAKE::loadAudio()
{
	// audio accessor is unusuable/bugged unless channel mode is 0
	int initial_chanmode = getChannelMode();
	setChannelMode(0);

	vector<double> buffer(takeSamples, 0);
	AudioAccessor* accessor = CreateTakeAudioAccessor(takePtr);
	GetAudioAccessorSamples(accessor, audioFile.getSampleRate(), audioFile.getNumChannels(), audiobuf_starttime, takeFrames, buffer.data());
	DestroyAudioAccessor(accessor);

	setChannelMode(initial_chanmode);

	takeAudioBuffer.setSource(InterleavedToMultichannel(buffer.data(), audioFile.getNumChannels(), takeFrames), audioFile.getSampleRate(), audioFile.getBitDepth());
}

void TAKE::unloadAudio() { takeAudioBuffer.clear(); }

bool TAKE::isAudioInitialized() { return audioIsInitialized; }

int TAKE::getSampleRate() { return audioFile.getSampleRate(); }

int TAKE::getBitDepth() { return audioFile.getBitDepth(); }

int TAKE::getNumChannels() { return audioFile.getNumChannels(); }

int TAKE::getNumChannelModeChannels()
{
	return getLastChannel() - getFirstChannel() + 1;
}

size_t TAKE::getNumFrames() const { return takeFrames; }

size_t TAKE::getNumSamples() const { return takeSamples; }

vector<vector<double>> & TAKE::getAudioMultichannel() { return takeAudioBuffer.getData(); }

vector<double>& TAKE::getAudioChannel(int channel)
{
	return takeAudioBuffer[channel];
}

double TAKE::getSample(int channel, int frame)
{
	return takeAudioBuffer[channel][frame];
}

double TAKE::getProjectPositionForFrameIndex(int index)
{
	return getStart() + index / getSampleRate();
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

		double offset = ITEM(take->getMediaItemPtr()).getStart();

		MIDI_GetNote(take->getPointer(), n, &selected, &muted, &ppqStart, &ppqEnd, &channel, &note, &vel);

		double startTime = MIDI_GetProjTimeFromPPQPos(take->getPointer(), ppqStart) - offset;
		double endTime = MIDI_GetProjTimeFromPPQPos(take->getPointer(), ppqEnd) - offset;

		push_back({ note, startTime, endTime, vel, channel, muted, selected });
		back().take = take;
		back().index = n;
	}
}

void MIDINOTELIST::insert(MIDINOTE obj)
{
	if (obj.take == nullptr)
		obj.take = take;

	double position = obj.getProjectStart();

	double ppq_start = MIDI_GetPPQPosFromProjTime(take->getPointer(), position);
	double ppq_end = MIDI_GetPPQPosFromProjTime(take->getPointer(), position + obj.getLength());

	bool noSort = false;
	MIDI_InsertNote(take->getPointer(), obj.selected, obj.muted, ppq_start, ppq_end, obj.channel, obj.pitch, obj.velocity, &noSort);
}

void MIDINOTELIST::append(MIDINOTELIST list)
{
	for (const auto& note : list)
		insert(note);

	MIDI_Sort(take->getPointer());
}

void MIDINOTELIST::remove(int index)
{
	MIDI_DeleteNote(take->getPointer(), index);
}

void MIDINOTELIST::removeAll()
{
	int notecount;
	MIDI_CountEvts(take->getPointer(), &notecount, nullptr, nullptr);
	for (int i = notecount; i --> 0; )
		MIDI_DeleteNote(take->getPointer(), i);
}

void MIDINOTE::setPitch(int v)
{
	bool noSort = false;
	MIDI_SetNote(take->getPointer(), index, nullptr, nullptr, nullptr, nullptr, nullptr, &v, nullptr, &noSort);

	pitch = v;
}

void MIDINOTE::setVelocity(int v)
{
	bool noSort = false;
	MIDI_SetNote(take->getPointer(), index, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &v, &noSort);

	velocity = v;
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

double MIDINOTE::getProjectStart() const { return startTime + ITEM(take->getMediaItemPtr()).getStart() + take->getStartOffset(); }

double MIDINOTE::getProjectEnd() const { return endTime + ITEM(take->getMediaItemPtr()).getStart() + take->getStartOffset(); }

vector<TAKEMARKER> TAKEMARKER::collect(TAKE& take)
{
	vector<TAKEMARKER> list;

	int numMarkers = TAKEMARKER::count(take);
	for (int i = 0; i < numMarkers; ++i)
		list.push_back(TAKEMARKER::getByIdx(take, i));

	return std::move(list);
}

void TAKEMARKER::replace(TAKE& take, vector<TAKEMARKER>& list)
{
	int i = 0;
	while (DeleteTakeMarker(take.getPointer(), i))
		++i;

	for (auto& m : list)
		TAKEMARKER::add(take, m.position, m.name, m.color);
}

int TAKEMARKER::count(TAKE& take)
{
	return GetNumTakeMarkers(take.getPointer());
}

TAKEMARKER TAKEMARKER::add(TAKE& take, double position, String name, int color)
{
	int idx = SetTakeMarker(take.getPointer(), -1, (char*)name.toRawUTF8(), &position, &color);
	return { take, idx, position, name, color };
}

TAKEMARKER TAKEMARKER::addOrUpdateByName(TAKE& take, double position, String name, int color)
{
	auto list = TAKEMARKER::collect(take);

	auto it = std::find_if(std::begin(list), std::end(list), [&](TAKEMARKER& m) { return m.name == name; });

	int i = -1;
	if (it != std::end(list))
		i = it->getIndex();

	int idx = SetTakeMarker(take.getPointer(), i, (char*)name.toRawUTF8(), &position, &color);
	return { take, idx, position, name, color };
}

TAKEMARKER TAKEMARKER::addOrUpdateByIdx(TAKE& take, int idx, double position, String name, int color)
{
	auto list = TAKEMARKER::collect(take);

	idx = SetTakeMarker(take.getPointer(), idx, (char*)name.toRawUTF8(), &position, &color);
	return { take, idx, position, name, color };
}

TAKEMARKER TAKEMARKER::getByName(TAKE& take, String name)
{
	if (name.isNotEmpty())
	{
		auto list = TAKEMARKER::collect(take);

		auto it = std::find_if(std::begin(list), std::end(list), [&](TAKEMARKER& m) { return m.name == name; });

		int i = -1;

		char c[256];
		String name;
		int color;
		double position;
		if (it != std::end(list))
			return *it;
	}

	return TAKEMARKER{ take, -1, 0 };
}

TAKEMARKER TAKEMARKER::getByIdx(TAKE& take, int idx)
{
	char c[256];
	String name;
	double position = 0;
	int color = 0;

	if (isPositiveAndBelow(idx, TAKEMARKER::count(take)))
	{
		position = GetTakeMarker(take.getPointer(), idx, c, 256, &color);
		name = c;
	}
	else
	{
		idx = -1;
		jassertfalse;
	}

	return { take, idx, position, name, color };
}

bool TAKEMARKER::remove()
{
	idx = -1;
	return DeleteTakeMarker(take->getPointer(), idx);
}

String TAKEMARKER::getName()
{
	int sz = 256;
	char c[256];
	for (int i = 0; i < 256; ++i)
		c[i] = 0;
	GetTakeMarker(take->getPointer(), idx, c, sz, nullptr);

	String ret = c;

	return c;
}

void TAKEMARKER::setName(const String& v)
{
	SetTakeMarker(take->getPointer(), idx, (char*)v.toRawUTF8(), nullptr, nullptr);
}

Colour TAKEMARKER::getColor()
{
	GetTakeMarker(take->getPointer(), idx, nullptr, 0, &color);
	return reaperToJuceColor(color);
}

void TAKEMARKER::setColor(const Colour& v)
{
	color = juceToReaperColor(v);
	SetTakeMarker(take->getPointer(), idx, nullptr, nullptr, &color);
}

double TAKEMARKER::getPosition()
{
	position = GetTakeMarker(take->getPointer(), idx, nullptr, 0, nullptr);
	return position;
}

void TAKEMARKER::setPosition(double v)
{
	position = v;
	idx = SetTakeMarker(take->getPointer(), idx, nullptr, &v, nullptr);
}
