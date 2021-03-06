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

	static bool write(const File& path, vector<double> singleChannelAudio, int sampleRate, int bitDepth);
	static bool write(const File& path, vector<vector<double>> multichannelAudio, int sampleRate, int bitDepth);

	WavAudioFile(const File& sourceFile);

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

	struct FileInfo
	{
		double length = 0;
		double sampleRate = 0;
		int numChannels = 0;
		int numFrames = 0;
		int numSamples = 0;
		int bitDepth = 0;
		int fileLength = 0;
		File path;
	} fileInfo;

	struct AudioInfo
	{
		int startSample = 0;
		double sampleRate = 0;
		int bitDepth = 0;
		int numChannels = 0;
		int numFrames = 0;
		double gain = 1;
	} clipInfo;

	void readFileInfo()
	{
		fileInfo.sampleRate = 0;
		fileInfo.bitDepth = 0;
		fileInfo.numFrames = 0;
		fileInfo.numChannels = 0;
		fileInfo.numSamples = 0;
		fileInfo.length = 0;

		audioFileWasRead = false;

		reader = manager.createReaderFor(fileInfo.path);

		audioFileWasRead = reader != nullptr;

		if (!isValid())
		{
			jassertfalse;
			return;
		}

		fileInfo.sampleRate = reader->sampleRate;
		fileInfo.bitDepth = reader->bitsPerSample;
		fileInfo.numFrames = reader->lengthInSamples;
		fileInfo.numChannels = reader->numChannels;
		fileInfo.numSamples = reader->lengthInSamples * reader->numChannels;
		fileInfo.length = reader->lengthInSamples / reader->sampleRate;
	}

	void setClipInfoToFileInfo()
	{
		clipInfo.numChannels = fileInfo.numChannels;
		clipInfo.numFrames = fileInfo.numFrames;
		clipInfo.sampleRate = fileInfo.sampleRate;
		clipInfo.bitDepth = fileInfo.bitDepth;
	}

	void ensureAudioLoaded();
	/** Returns true if the source file was read correctly */
	bool isValid() const { return reader != nullptr && didRead; }

private:

  ScopedPointer<AudioFormatReader> reader;

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
	bool audioFileWasRead = false;

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

vector<vector<double>> InterleavedToMultichannel(double* input, int channels, int frames);
