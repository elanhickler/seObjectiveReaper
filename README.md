# seObjectiveReaper
C++ library for using REAPER API through an object oriented interface. Warning: Large changes will be made to continue to make this library easy to use.

# Main Classes

#### PROJECT 
Functions to manipulate the REAPER project.

#### AUDIODATA
Functions to read/write/manipulate audio eiter from TAKE or arbitrary data.

#### MIDINOTE
Functions to manipulate a midi note or to create new midi notes in TAKE.

#### MIDINOTELIST
Holds a list of MIDINOTE to manipulate TAKE midi.

#### TAKE
Functions to manipulate a `MediaItem_Take*` and access take audio. Owns an AUDIODATA object.

#### TAKELIST
Functions to gather takes. Holds a list of TAKE.

### ITEM
Functions to manipulate a `MediaItem*`. Owns a TAKELIST.

#### ITEMLIST
Functions to gather items. Holds a list of ITEM.

#### ITEMGROUPLIST
Functions to gather groups of items based on item grouping, or if overlapping or touching. Holds a list of ITEMLIST.

#### TRACK
Functions to manipulate `MediaTrack*`.

#### TRACKLIST
Functions to gather tracks. Holds a list of TRACK.

#### MARKER
Functions to manipulate project markers and regions.

#### MARKERLIST
Functions to gather project markers and regions. Holds a list of MARKER.

#### STRETCHMAKER
Functions to manipulate stretch markers inside a `MediaTake*`.

#### STRETCHMAKERLIST
Functions to gather stretch markers. Holds a list of STRETCHMARKER.

#### ENVPT
Functions to manipulate a point in ENVELOPE.

#### ENVELOPE
Functions to manipulate a TrackEnvelope*. Holds a list of ENVPT. 

#### AUTOITEM
Functions to manpipulate automation items.

#### AUTOITEMLIST
Functions to gather automation items. Holds a list of AUTOITEM.
