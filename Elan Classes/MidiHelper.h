#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

using juce::juce_wchar;

struct char_int { juce_wchar c; int i; };
struct str_int { String s; int i; };

class MIDI
{
public:
  MIDI();
  MIDI(String s, int transpose = 0);
  MIDI(int i, int transpose = 0);

  void read(int i, int transpose = 0);
  void read(String s, int transpose = 0);

  void setMidOct(int i);

  void format(const String& s);
  int getNum();
  int getNum(const String & name);
  String getName();
  int getName(int num);
  String applyOffset(int i);

private:
  int lowest_octave = -2;

  int midi_number;
  String midi_name;

  // value representations
  int root;
  int accidental;
  int multiplier;
  int sign;
  int octave;

  bool use_flat = false;
  bool use_capital_root = true;
  bool use_capital_accidental = false;
  bool always_show_sign = false;
};