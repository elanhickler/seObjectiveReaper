#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

using namespace juce;

using std::vector;


/**
* WAV file meta-data decoder.
*
* @notes
* This object could be expanded to preserve or allow editing of other types of WAV metadata
*/
class WavMetadata
{
public:
  struct CuePoint
  {
    String label;
    int64 offset; // in samples

  private:
    friend class WavMetadata;
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
    friend class WavMetadata;
    int identifier{ -1 }; // internal wav file use for linking regions with cue points
  };

  struct Loop
  {
    enum LoopType
    {
      kForward,
      kPingPong,
      kBackward
    } type;

    int start{ 0 };
    int end{ 0 };
    int fraction{ 0 };
    int playcount{ 0 };

    int cueIdentifier{ -1 };
  };


  /**
  * Create a WavMetadata object. Once this is created the regions and cuepoints
  * Array's can be inspected and modified as required.  Then call saveChanges
  * to write your changes to the WAV metadata to disk.  Metadata that is not
  * supported by this object will be removed.
  */
  static WavMetadata * create(const File & file)
  {
    ScopedPointer<WavMetadata> wavAudioFile = new WavMetadata(file);

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

  /**
  * Write the changes to the audio file to disk.  Returns false if the file could not be
  * written.
  */
  bool saveChanges(const File & destination);

  /** Returns a copy of the metadata as read from the original file.  Used for debugging. */
  StringPairArray getMetadata() const;
private:
  WavMetadata(const File& file);
  bool isValid() const { return didRead; }

  String getStringValue(const String& prefix, int num, const String& postfix) const;
  int64 getIntValue(const String& prefix, int num, const String& postfix) const;

  void loadMetaData();
  void createNewRegions();
  void addAdditionalCuePoints();
  void createNewCuePoints();
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
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavMetadata)
};

class AUDIO
{
public:
    WavMetadata * cues;

    AUDIO () {}
    AUDIO(File file) : m_file(file) {}

    File m_file;
    int m_srate;
    int m_bitdepth;
    int m_frames;
    int m_samples;
    int m_channels;
    double m_length;    
    vector<vector<double>> multichannel;

    // functions
    void setFile(File file) 
    { 
        m_file = file; 
    }
    void collectCues();
    Array<WavMetadata::CuePoint> getCuePoints();
    Array<WavMetadata::Region> getCueRegions();
    Array<WavMetadata::Loop> getLoops();
    void writeCues();

    // getters
    int samples() { return m_samples; }
    int channels() { return m_channels; }
    int frames() { return m_frames; }
    int srate() { return m_srate; }
    int bitdepth() { return m_bitdepth; }
    double lenght() { return m_length; }
};

vector<vector<double>> InterleavedToMultichannel(double* input, int channels, int frames);
