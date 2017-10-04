#include <vector>
using std::vector;
#include "Audio.h"

vector<vector<double>> InterleavedToMultichannel(double* input, int channels, int frames)
{
    vector<vector<double>> data(channels, vector<double>(channels));

    for (auto& c : data) c.reserve(frames);

    for (int ch = 0; ch < channels; ++ch)
        for (int x = 0, y = ch; x < frames; ++x, y+=channels)
            data[ch].push_back(input[y]);

    return data;
}

static const String idCueLabel{ "CueLabel" };
static const String idCueRegion{ "CueRegion" };
static const String idCue{ "Cue" };
static const String idLoop{ "Loop" };


WavAudioFile::WavAudioFile(const File& sourceFile, double startOffset, double length)
  :
  clipStart(startOffset), clipLength(length)
{
  manager.registerBasicFormats();

  reader = manager.createReaderFor(sourceFile);
  sampleRate = reader->sampleRate;

  if (reader)
  {
    metadata = reader->metadataValues;
    loadMetaData();
    didRead = true;
  }
}

StringPairArray WavAudioFile::getMetadata() const
{
  return metadata;
}

double WavAudioFile::getSampleRate() const
{
  return reader->sampleRate;
}

//void WavAudioFile::setSampleStart(int s)
//{
//	sampleStart = s;
//}
//void WavAudioFile::setSampleEnd(int s)
//{
//	sampleEnd = s;
//}

double WavAudioFile::getFileLengthInSeconds() const
{
  if (reader->sampleRate == 0.0)
    return 0.0;

  return double(reader->lengthInSamples) / reader->sampleRate;
}

int64 WavAudioFile::getLengthInSamples() const
{
  return reader->lengthInSamples;
}

void WavAudioFile::loadAudio()
{
  auto startSample = getSamplePosition(clipStart);
  auto numSamples = getSamplePosition(clipLength);

  jassert(numSamples != 0);
  jassert(clipStart + numSamples <= int(reader->lengthInSamples));

  audio = new AudioSampleBuffer(reader->numChannels, numSamples);
  reader->read(audio, 0, numSamples, startSample, true, true);
}

int WavAudioFile::getSamplePosition(double timeInSeconds) const
{
  return int(std::round(timeInSeconds * sampleRate));
}

//void WavAudioFile::applyFadeOutAtEnd(int fadeLength, int shape, double tension)
//{
//	ensureAudioLoaded();
//
//	if (audio->getNumChannels() == 0 || audio->getNumSamples() < fadeLength)
//	{
//		jassertfalse;
//		return;
//	}
//
//	auto data = audio->getArrayOfWritePointers();
//	auto numSamples = audio->getNumSamples();
//	auto numChans = audio->getNumChannels();
//	int count{ fadeLength };
//
//	for (int i = numSamples - fadeLength; i < numSamples; ++i)
//	{
//		auto fadeAmount = float(count) / float(fadeLength);
//
//		for (int chan = 0; chan < numChans; ++chan)
//			data[chan][i] *= fadeAmount;
//
//		count--;
//	}
//}

double WavAudioFile::getPeakValue()
{
  ensureAudioLoaded();

  auto data = audio->getArrayOfReadPointers();
  auto numSamples = audio->getNumSamples();
  auto numChans = audio->getNumChannels();

  double v = 0.0;

  for (int chan = 0; chan < numChans; ++chan)
    for (int i = 0; i < numSamples; ++i)
      v = max<double>(v, std::fabs(data[chan][i]));

  return v;
}

void WavAudioFile::ensureAudioLoaded()
{
  if (audio == nullptr)
    loadAudio();
}

int WavAudioFile::getNumSampleLoops() const
{
  return metadata["NumSampleLoops"].getIntValue();
}

int WavAudioFile::getNumCueRegions() const
{
  return metadata["NumCueRegions"].getIntValue();
}

int WavAudioFile::getNumCueLabels() const
{
  return metadata["NumCueLabels"].getIntValue();
}

int WavAudioFile::getNumCuePoints() const
{
  return metadata["NumCuePoints"].getIntValue();
}

bool WavAudioFile::saveChanges(const File& destination)
{
  if (destination == sourceFile)
  {
    Random random;
    File newSource;

    do { newSource = sourceFile.getSiblingFile("tmp_" + String(random.nextInt64()) + ".wav"); } while (newSource.existsAsFile());

    auto result = sourceFile.moveFileTo(newSource);

    if (!result)
      return false;

    sourceFile = newSource;
  }

  createMetaDataFromArrays();

  //if (destination.existsAsFile())
  //	return false;

  ensureAudioLoaded();
  //ScopedPointer<AudioFormatReader> reader = manager.createReaderFor(sourceFile);

  if (reader)
  {
    WavAudioFormat wavFormat;

    ScopedPointer<AudioFormatWriter> writer = wavFormat.createWriterFor(new FileOutputStream(destination),
      reader->sampleRate, reader->numChannels, reader->bitsPerSample, newMetadata, 0);

    if (!writer)
      return false;

    if (audio)
    {
      auto result = writer->writeFromAudioSampleBuffer(*audio, 0, audio->getNumSamples());

      if (!result)
        return false;
    }
    else
    {
      jassertfalse; // we must have audio loaded.
      return false;
    }
  }
  else
  {
    return false;
  }

  return true;
}

String WavAudioFile::getStringValue(const String& prefix, int num, const String& postfix) const
{
  return metadata[prefix + String(num) + postfix];
}

int64 WavAudioFile::getIntValue(const String& prefix, int num, const String& postfix) const
{
  return metadata[prefix + String(num) + postfix].getLargeIntValue();
}

void WavAudioFile::loadMetaData()
{
  //for (int i = 0; i < getNumSampleLoops(); ++i)
  //	loops.add(getSampleLoop(i));

  for (int i = 0; i < getNumCueRegions(); ++i)
    regions.add(getCueRegion(i));

  for (int i = 0; i < getNumCuePoints(); ++i)
  {
    bool alreadyPresent{ false };

    auto cuePoint = getCuePoint(i);

    for (auto &r : regions)
      if (r.label == cuePoint.label && r.offset == cuePoint.offset)
        alreadyPresent = true;

    if (!alreadyPresent)
      cuePoints.add(getCuePoint(i));
  }

  for (int i = 0; i < getNumSampleLoops(); ++i)
    loops.add(getSampleLoop(i));
}

void WavAudioFile::createNewRegions()
{
  int regionId{ 0 };

  for (auto& r : regions)
  {
    CuePoint cue;
    cue.offset = r.offset;
    cue.label = r.label;
    newCues.add(cue);

    newMetadata.set(idCueRegion + String(regionId) + "Identifier", String(newCues.size()));
    newMetadata.set(idCueRegion + String(regionId) + "SampleLength", String(r.length));
    newMetadata.set(idCueRegion + String(regionId) + "SampleLength", String(r.length));

    regionId++;
  }

  newMetadata.set("NumCueRegions", String(regionId));
}

void WavAudioFile::addAdditionalCuePoints()
{
  for (auto & cue : cuePoints)
    newCues.add(cue);
}

void WavAudioFile::createNewCuePoints()
{
  int cueIndex{ 0 };

  for (auto & r : newCues)
  {
    String baseCue = idCue + String(cueIndex);

    /* JUCE applies default values for data we don't provide here, e.g. ChunkID */
    newMetadata.set(baseCue + "Identifier", String(cueIndex + 1));
    newMetadata.set(baseCue + "Offset", String(r.offset));
    newMetadata.set(baseCue + "Order", String(r.offset));

    String baseLabel = idCueLabel + String(cueIndex);

    newMetadata.set(baseLabel + "Identifier", String(cueIndex + 1));
    newMetadata.set(baseLabel + "Text", r.label);

    cueIndex++;
  }

  newMetadata.set("NumCuePoints", String(cueIndex));
  newMetadata.set("NumCueLabels", String(cueIndex));
}

void WavAudioFile::createNewLoops()
{
  int sampleLoopId{ 0 };

  for (auto& l : loops)
  {
    CuePoint cue;
    cue.offset = l.start;
    cue.label = l.label;
    newCues.add(cue);

    newMetadata.set(idLoop + String(sampleLoopId) + "Identifier", String(newCues.size()));
    newMetadata.set(idLoop + String(sampleLoopId) + "PlayCount", String(l.playcount));
    newMetadata.set(idLoop + String(sampleLoopId) + "Fraction", String(l.fraction));
    newMetadata.set(idLoop + String(sampleLoopId) + "Type", String(int(l.type)));
    newMetadata.set(idLoop + String(sampleLoopId) + "Start", String(l.start));
    newMetadata.set(idLoop + String(sampleLoopId) + "End", String(l.end));

    sampleLoopId++;
  }

  newMetadata.set("NumSampleLoops", String(sampleLoopId));
}

void WavAudioFile::createMetaDataFromArrays()
{

  newMetadata.clear();
  newCues.clear();

  createNewRegions();
  createNewLoops();
  addAdditionalCuePoints();
  createNewCuePoints();
}

WavAudioFile::CuePoint WavAudioFile::getCuePoint(int index) const
{
  auto base = String("Cue") + String(index);

  CuePoint r;
  r.offset = getIntValue(idCue, index, "Offset");
  r.identifier = getIntValue(idCue, index, "Identifier");

  // we also have ChunkID which should normally, it seems, be 'data'.
  jassert(getIntValue(idCue, index, "ChunkID") == 1635017060L);

  // appears to be zero ... need to check this one...
  jassert(getIntValue(idCue, index, "ChunkStart") == 0L);

  // The dwBlockStart field specifies the byte offset of the start of the block containing the 
  // position. This offset is relative to the start of the waveform data within the 'data' or 'slnt' chunk.
  jassert(getIntValue(idCue, index, "BlockStart") == 0L);

  r.label = getCueLabel(r.identifier);

  return r;
}

WavAudioFile::CuePoint WavAudioFile::getCuePointByIdentifier(int identifier) const
{
  for (int i = 0; i < getNumCuePoints(); ++i)
  {
    auto cp = getCuePoint(i);

    if (cp.identifier == identifier)
      return cp;
  }

  jassertfalse; // cue point not found
  return{};
}

String WavAudioFile::getCueLabel(int identifier) const
{
  auto numLabels = getNumCueLabels();

  for (int i = 0; i < numLabels; ++i)
    if (getIntValue(idCueLabel, i, "Identifier") == identifier)
      return getStringValue(idCueLabel, i, "Text");

  return String();
}

WavAudioFile::Region WavAudioFile::getCueRegion(int index) const
{
  Region r;
  r.identifier = getIntValue(idCueRegion, index, "Identifier");
  r.length = getIntValue(idCueRegion, index, "SampleLength");

  auto cuePoint = getCuePointByIdentifier(r.identifier);

  r.offset = cuePoint.offset;
  r.label = cuePoint.label;
  return r;
}

WavAudioFile::Loop WavAudioFile::getSampleLoop(int index) const
{
  Loop r;
  r.start = getIntValue(idLoop, index, "Start");
  r.type = Loop::LoopType(getIntValue(idLoop, index, "Type"));
  r.end = getIntValue(idLoop, index, "End");
  r.fraction = getIntValue(idLoop, index, "Fraction");
  r.playcount = getIntValue(idLoop, index, "PlayCount");
  r.cueIdentifier = getIntValue(idLoop, index, "Identifier");
  return r;
}/*

void AudioFile::collectCues()
{
    cues = WavAudioFile::create(m_file);
}

Array<WavAudioFile::CuePoint> AudioFile::getCuePoints()
{
    return cues->cuePoints;
}

Array<WavAudioFile::Region> AudioFile::getCueRegions()
{
    return cues->regions;
}

Array<WavAudioFile::Loop> AudioFile::getLoops()
{
  return cues->loops;
}

void AudioFile::writeCues()
{
    cues->saveChanges(m_file);
}*/
