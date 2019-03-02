#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include "../reaper plugin/reaper_plugin_functions.h"

using namespace juce;

using std::vector;


/**
* WAV file meta-data decoder.
*
* @notes
* This object could be expanded to preserve or allow editing of other types of WAV metadata
*/
class WavAudioFile
{
public:
  struct CuePoint
  {
    String label;
    int64 offset; // in samples

  private:
    friend class WavAudioFile;
    int identifier{ -1 }; // internal wav file use for linking labels with regions
  };

  /**
  * Using LTXT chunk data.
  */
  struct Region
  {
    String label;
    int64 offset{ 0 };
    int64 length{ 0 };

  private:
    friend class WavAudioFile;
    int identifier{ -1 }; // internal wav file use for linking regions with cue points
  };

  struct Loop
  {
    enum LoopType
    {
      kForward,
      kPingPong,
      kBackward
    } type{ kForward };

    String label;

    int start{ 0 };
    int end{ 0 };
    int fraction{ 0 };
    int playcount{ 0 };

    int cueIdentifier{ -1 };
  };



  /**
  * Create a WavAudioFile object. Once this is created the regions and cuepoints
  * Array's can be inspected and modified as required.  Then call saveChanges
  * to write your changes to the WAV metadata to disk.  Metadata that is not
  * supported by this object will be removed.
  */
  static WavAudioFile * create(const File & file, double startTime, double lengthInSeconds)
  {
    ScopedPointer<WavAudioFile> wavAudioFile = new WavAudioFile(file, startTime, lengthInSeconds);

    if (wavAudioFile->isValid())
      return wavAudioFile.release();

    return nullptr;
  }

  /** Contains cue point, region and label information for all complete regions. */
  Array<Region> regions;

  /** Contains all stand-alone cue points and labels (i.e. those that aren't associated with a region. */
  Array<CuePoint> cuePoints;

  /** Contains all the loops from the loaded sample file.  We do not support writing loop data at present */
  Array<Loop> loops;

  ScopedPointer<AudioSampleBuffer> audio;

  /**
  * Write the changes to the audio file to disk.  Returns false if the file could not be
  * written.
  */
  bool saveChanges(const File & destination);

  WavAudioFile(const File& sourceFile, double startOffset, double length);
  /** Returns a copy of the metadata as read from the original file.  Used for debugging. */
  StringPairArray getMetadata() const;

  double getSampleRate() const;
  double getFileLengthInSeconds() const;
  int64 getLengthInSamples() const;

  //void setSampleStart(int s);
  //void setSampleEnd(int s);

  /**
  * Remove all loops, cuepoints and regions.
  */
  void clearAllMetadata()
  {
    loops.clear();
    regions.clear();
    cuePoints.clear();
  }

  void loadAudio();

  int getSamplePosition(double timeInSeconds) const;

  /**
  * Adjust the start time of the sample relative to the start of the
  * audio file (the sample offset)
  */
  void setStartTimeInSeconds(double newStart)
  {
    if (newStart != clipStart && audio != nullptr)
      loadAudio();

    clipStart = newStart;
  }

  void setLengthInSeconds(double newLength)
  {
    jassert(newLength != 0.0);

    if (newLength != clipLength && audio != nullptr)
      loadAudio();

    clipLength = newLength;
  }

  /**
  * Audio functions
  */

  enum FadeShape { LINEAR, EXPONENTIAL, S_CURVE, S_CURVE_EXTREME };

  //void applyFadeOutAtEnd(int fadeLength, int shape = 0, double tension = 0);

  double getPeakValue();


  double gain = 1;

  double dBToAmp(double v) { return pow(10.0, 0.05 * v); }
  double ampTodB(double v) { return 20 * log10(v); }

  void setGainIndB(double v)
  {
    gain = dBToAmp(v);
  }

private:

  void ensureAudioLoaded();
  /** Returns true if the source file was read correctly */
  bool isValid() const { return reader != nullptr && didRead; }

  ScopedPointer<AudioFormatReader> reader;

  WavAudioFile(const File& file);

  String getStringValue(const String& prefix, int num, const String& postfix) const;
  int64 getIntValue(const String& prefix, int num, const String& postfix) const;

  void loadMetaData();
  void createNewRegions();
  void addAdditionalCuePoints();
  void createNewCuePoints();
  void createNewLoops();
  void createMetaDataFromArrays();

  int getNumSampleLoops() const;
  Loop getSampleLoop(int index) const;

  int getNumCueRegions() const;
  Region getCueRegion(int index) const;

  int getNumCueLabels() const;
  String getCueLabel(int index) const;

  int getNumCuePoints() const;
  CuePoint getCuePoint(int index) const;
  CuePoint getCuePointByIdentifier(int identifier) const;

  bool didRead{ false };

  AudioFormatManager manager;

  /* input buffer ... */
  StringPairArray metadata;

  /* output buffers... */
  StringPairArray newMetadata;
  Array<CuePoint> newCues;
  File sourceFile;

  double clipStart;
  double clipLength;
  double sampleRate{ 1.0 };

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavAudioFile)
};

class AudioFile
{
public:
  WavAudioFile * cues;

  AudioFile() {}

	vector<double>& operator[](int i) { return multichannel[i]; }
	vector<double> operator[](int i) const { return multichannel[i]; }

	AudioFile(const vector<vector<double>> & multichannelAudio, int sampleRate, int bitDepth)
	{
		setSource(multichannelAudio, sampleRate, bitDepth);
	}

	AudioFile(const vector<vector<float>> & multichannelAudio, int sampleRate, int bitDepth)
	{
		setSource(convertAudioType<double>(multichannelAudio), sampleRate, bitDepth);
	}

	AudioFile(PCM_source* source)
	{
		setSource(source);
	}

	AudioFile(const File & file)
	{
		setSource(file);
	}

  void setSource(PCM_source* source)
  {
    file = source->GetFileName();
		srate = source->GetSampleRate();
		channels = source->GetNumChannels();
		length = source->GetLength();
		samples = int(source->GetLength() * channels * srate);
		bitdepth = source->GetBitsPerSample();
		frames = source->GetLength() * srate;
  }

	void setSource(const File & file)
	{
		setSource(PCM_Source_CreateFromFile(file.getFullPathName().toRawUTF8()));
	}

	void setSource(const vector<vector<double>> multichannelAudio, int sampleRate, int bitDepth)
	{
		multichannel = multichannelAudio;
		srate = sampleRate;
		bitdepth = bitDepth;
		channels = multichannel.size();
		length = multichannel[0].size() / sampleRate;
		frames = multichannel[0].size();
		samples = int(frames * channels * srate);
	}

	void writeToFile(const File & file) const
	{
		AudioSampleBuffer buffer = convertToAudioSampleBuffer();

		WavAudioFormat format;
		std::unique_ptr<AudioFormatWriter> writer;
		writer.reset(format.createWriterFor(new FileOutputStream(file), srate, buffer.getNumChannels(), bitdepth, {}, 0));
		if (writer != nullptr)
			writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
	}

  void collectCues();
  Array<WavAudioFile::CuePoint> getCuePoints();
  Array<WavAudioFile::Region> getCueRegions();
  Array<WavAudioFile::Loop> getLoops();
  void writeCues();

  // getters
	File getFile() const { return file; }
	double getSample(int channel, int sample) { return multichannel[channel][sample]; }
	vector<double> & getChannel(int channel) { return multichannel[channel]; }
  int getNumSamples() const { return samples; }
  int getNumChannels() const { return channels; }
  int getNumFrames() const { return frames; }
  int getSampleRate() const { return srate; }
  int getBitDepth() const { return bitdepth; }
  double getLength() const { return length; }

	void clear()
	{
		multichannel.clear();
		file = File();
		int srate = 0;
		int bitdepth = 0;
		int frames = 0;
		int samples = 0;
		int channels = 0;
		double length = 0;
	}

protected:
	File file;
	int srate;
	int bitdepth;
	int frames;
	int samples;
	int channels;
	double length;
	vector<vector<double>> multichannel;

	AudioSampleBuffer convertToAudioSampleBuffer() const
	{
		vector<vector<float>> audio = convertAudioType<float>(multichannel);

		AudioSampleBuffer buffer(getNumChannels(), getNumFrames());
		for (int ch = 0; ch < getNumChannels(); ++ch)
			buffer.addFrom(ch, 0, audio[ch].data(), audio[ch].size());

		return std::move(buffer);
	}

	template <typename t1, typename t2> vector<vector<t1>> convertAudioType(const vector<vector<t2>> & type2Audio) const
	{
		vector<vector<t1>> type1Audio;

		type1Audio.reserve(type2Audio.size());
		for (int ch = 0; ch < type2Audio.size(); ++ch)
			type1Audio.push_back({ type2Audio[ch].begin(), type2Audio[ch].end() });

		return std::move(type1Audio);
	}
};

vector<vector<double>> InterleavedToMultichannel(double* input, int channels, int frames);
