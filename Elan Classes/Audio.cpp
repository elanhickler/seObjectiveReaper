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


WavMetadata::WavMetadata(const File& sourceFile)
  :
  sourceFile(sourceFile)
{
  manager.registerBasicFormats();

  ScopedPointer<AudioFormatReader> reader = manager.createReaderFor(sourceFile);

  if (reader)
  {
    metadata = reader->metadataValues;
    loadMetaData();
    didRead = true;
  }
}

StringPairArray WavMetadata::getMetadata() const
{
  return metadata;
}

int WavMetadata::getNumSampleLoops() const
{
  return metadata["NumSampleLoops"].getIntValue();
}

int WavMetadata::getNumCueRegions() const
{
  return metadata["NumCueRegions"].getIntValue();
}

int WavMetadata::getNumCueLabels() const
{
  return metadata["NumCueLabels"].getIntValue();
}

int WavMetadata::getNumCuePoints() const
{
  return metadata["NumCuePoints"].getIntValue();
}

bool WavMetadata::saveChanges(const File& destination)
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

  ScopedPointer<AudioFormatReader> reader = manager.createReaderFor(sourceFile);

  if (reader)
  {
    WavAudioFormat wavFormat;

    ScopedPointer<AudioFormatWriter> writer = wavFormat.createWriterFor(new FileOutputStream(destination),
      reader->sampleRate, reader->numChannels, reader->bitsPerSample, newMetadata, 0);

    if (!writer)
      return false;

    auto result = writer->writeFromAudioReader(*reader, 0, reader->lengthInSamples);

    if (!result)
      return false;
  }
  else
  {
    return false;
  }

  return true;
}

String WavMetadata::getStringValue(const String& prefix, int num, const String& postfix) const
{
  return metadata[prefix + String(num) + postfix];
}

int64 WavMetadata::getIntValue(const String& prefix, int num, const String& postfix) const
{
  return metadata[prefix + String(num) + postfix].getLargeIntValue();
}

void WavMetadata::loadMetaData()
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

void WavMetadata::createNewRegions()
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

void WavMetadata::addAdditionalCuePoints()
{
  for (auto & cue : cuePoints)
    newCues.add(cue);
}

void WavMetadata::createNewCuePoints()
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

void WavMetadata::createMetaDataFromArrays()
{
  newMetadata.clear();
  newCues.clear();

  createNewRegions();
  addAdditionalCuePoints();
  createNewCuePoints();
}

WavMetadata::CuePoint WavMetadata::getCuePoint(int index) const
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

WavMetadata::CuePoint WavMetadata::getCuePointByIdentifier(int identifier) const
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

String WavMetadata::getCueLabel(int identifier) const
{
  auto numLabels = getNumCueLabels();

  for (int i = 0; i < numLabels; ++i)
    if (getIntValue(idCueLabel, i, "Identifier") == identifier)
      return getStringValue(idCueLabel, i, "Text");

  return String();
}

WavMetadata::Region WavMetadata::getCueRegion(int index) const
{
  Region r;
  r.identifier = getIntValue(idCueRegion, index, "Identifier");
  r.length = getIntValue(idCueRegion, index, "SampleLength");

  auto cuePoint = getCuePointByIdentifier(r.identifier);

  r.offset = cuePoint.offset;
  r.label = cuePoint.label;
  return r;
}

WavMetadata::Loop WavMetadata::getSampleLoop(int index) const
{
  Loop r;
  r.start = getIntValue(idLoop, index, "Start");
  r.type = Loop::LoopType(getIntValue(idLoop, index, "Type"));
  r.end = getIntValue(idLoop, index, "End");
  r.fraction = getIntValue(idLoop, index, "Fraction");
  r.playcount = getIntValue(idLoop, index, "PlayCount");
  r.cueIdentifier = getIntValue(idLoop, index, "Identifier");
  return r;
}

void AUDIO::collectCues()
{
    cues = WavMetadata::create(m_file);
}

Array<WavMetadata::CuePoint> AUDIO::getCuePoints()
{
    return cues->cuePoints;
}

Array<WavMetadata::Region> AUDIO::getCueRegions()
{
    return cues->regions;
}

Array<WavMetadata::Loop> AUDIO::getLoops()
{
  return cues->loops;
}

void AUDIO::writeCues()
{
    cues->saveChanges(m_file);
}
