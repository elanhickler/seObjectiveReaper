/*******************************************************************************
 The block below describes the properties of this module, and is read by
 the Projucer to automatically generate project code that uses it.
 For details about the syntax and how to create or use a module, see the
 JUCE Module Format.txt file.


 BEGIN_JUCE_MODULE_DECLARATION

  ID:               seObjectiveReaper
  vendor:           Soundemote
  version:          0.0.1
  name:             seObjectiveReaper
  description:      framework for using REAPER API
  website:          http://www.soundemote.com
  license:          WTFPL www.wtfpl.net

  dependencies:     juce_core, juce_audio_basics, juce_graphics, juce_gui_basics,
                    juce_audio_formats, juce_audio_processors, rapt, romos, rosic
  OSXFrameworks:
  iOSFrameworks:

 END_JUCE_MODULE_DECLARATION

*******************************************************************************/


#ifndef seObjectiveReaper_H_INCLUDED
#define seObjectiveReaper_H_INCLUDED

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "Elan Classes/ElanClassesHeader.h"
#include "Reaper Classes/ReaperClassesHeader.h"

#include "XenakiosStuff/jcomponents.h"

#endif
