#include "../reaper plugin/reaper_plugin_functions.h"

#include "ReaperClassesHeader.h"

#undef min
#undef max

extern reaper_plugin_info_t* g_plugin_info;

action_entry::action_entry(std::string description, std::string idstring, toggle_state togst, std::function<void(action_entry&)> func) :
	m_desc(description), m_id_string(idstring), m_func(func), m_togglestate(togst)
{
	if (g_plugin_info != nullptr)
	{
		m_accel_reg.accel = { 0,0,0 };
		m_accel_reg.desc = m_desc.c_str();
		m_accel_reg.accel.cmd = m_command_id = g_plugin_info->Register("command_id", (void*)m_id_string.c_str());
		g_plugin_info->Register("gaccel", &m_accel_reg);
	}
}

vector<MediaItem*> PROJECT::savedItems;
double PROJECT::saved_cursor_position;
bool PROJECT::view_is_being_saved = true;
double PROJECT::global_save_view_start = 0;
double PROJECT::global_save_view_end = 0;

void PROJECT::saveItemSelection()
{
	int items = PROJECT::countSelectedItems();

	savedItems.clear();
	savedItems.reserve(items);

	for (int i = 0; i < items; ++i)
		savedItems.push_back(GetSelectedMediaItem(0, i));
}

void PROJECT::loadItemSelection()
{
	unselectAllItems();
	for (const auto & item : savedItems)
		selectItem(item);
}

double PROJECT::setCursor(double time, bool moveview, bool seekplay)
{
	SetEditCurPos(time, moveview, seekplay);
	return time;
}
double PROJECT::getCursor() { return GetCursorPosition(); }

String PROJECT::getClipboardFile(String clipboardName)
{
	String fileName;
	if (clipboardName.isEmpty())
		fileName = "seSL.clipboard.txt";
	else
		fileName = "seSL." + clipboardName + ".clipboard.txt";

	String filePath = PROJECT::getDirectory() + "/" + fileName;
	return filePath;
}

void PROJECT::saveCursor() { saved_cursor_position = PROJECT::getCursor(); }
void PROJECT::loadCursor() { PROJECT::setCursor(saved_cursor_position); }

void PROJECT::saveView()
{
	GetSet_ArrangeView2(0, 0, 0, 0, &global_save_view_start, &global_save_view_end);
	view_is_being_saved = false;
}

void PROJECT::loadView()
{
	GetSet_ArrangeView2(0, 1, 0, 0, &global_save_view_start, &global_save_view_end);
	view_is_being_saved = true;
}

int juceToReaperColor(const Colour & v)
{
	return ColorToNative(v.getRed(), v.getGreen(), v.getBlue()) | 0x01000000;
}

Colour reaperToJuceColor(int v)
{
	int r, g, b;
	ColorFromNative(v, &r, &g, &b);
	return Colour(r, g, b);
}

double PROJECT::getEndTime()
{
	saveCursor();
	saveView();

	COMMAND(40043); // Transport: Go to end of project
	double time = getCursor();

	loadCursor();
	loadView();

	return time;
}

double PROJECT::getGridDivision()
{
	double division;
	GetSetProjectGrid(nullptr, false, &division, nullptr, nullptr);
	return division;
}

double PROJECT::getGridDivisionTime()
{
	return 120.0 / PROJECT::getTempo() * 2.0 * PROJECT::getGridDivision();
}

double PROJECT::getTempo()
{
	return Master_GetTempo();
}

void PROJECT::getTimeSignature(int& numerator, int& denominator)
{
	double bpm;
	double qnStart, qnEnd;
	TimeMap_GetMeasureInfo(nullptr, 0, &qnStart, &qnEnd, &numerator, &denominator, &bpm);
}

String PROJECT::getFilePath()
{
	char charbuf[1024];
	EnumProjects(-1, charbuf, 1024);
	return charbuf;
}

String PROJECT::getDirectory()
{
	return File(PROJECT::getFilePath()).getParentDirectory().getFullPathName();
}

String PROJECT::getName()
{
	return File(PROJECT::getFilePath()).getFileNameWithoutExtension();
}

int PROJECT::countMakersAndRegions()
{
	int m, r;
	CountProjectMarkers(nullptr, &m, &r);
	return m + r;
}

int PROJECT::countItems() { return CountMediaItems(0); }

int PROJECT::countTracks() { return CountTracks(0); }

int PROJECT::countSelectedTracks() { return CountSelectedTracks(0); }

void PROJECT::unselectItem(MediaItem * itemPtr)
{
	SetMediaItemInfo_Value(itemPtr, "B_UISEL", false);
}

bool ui_is_updating = true;
void UI()
{
	if (ui_is_updating)
	{
		PreventUIRefresh(1);
		ui_is_updating = false;
	}
	else
	{
		PreventUIRefresh(-1);
		ui_is_updating = true;
		UPDATE();
	}
}

bool undo_is_active = false;
void UNDO(String undostr, ReaProject* project)
{
	if (!undo_is_active)
		Undo_BeginBlock2(0);
	else
		Undo_EndBlock2(project, undostr.toRawUTF8(), -1);

	flip(undo_is_active);
}

void UPDATE()
{
	UpdateArrange();
	TrackList_AdjustWindows(false);
}



AUDIODATA::AUDIODATA(const vector<vector<double>>& multichannelAudio, int sampleRate, int bitDepth)
{
	setSource(multichannelAudio, sampleRate, bitDepth);
}

AUDIODATA::AUDIODATA(const vector<vector<float>>& multichannelAudio, int sampleRate, int bitDepth)
{
	setSource(convertAudioType<double>(multichannelAudio), sampleRate, bitDepth);
}

AUDIODATA::AUDIODATA(const vector<double>& singleChannelAudio, int sampleRate, int bitDepth)
{
	setSource(convertAudioType<double>(singleChannelAudio), sampleRate, bitDepth);
}

AUDIODATA::AUDIODATA(const vector<float>& singleChannelAudio, int sampleRate, int bitDepth)
{
	setSource(convertAudioType<double>(singleChannelAudio), sampleRate, bitDepth);
}

AUDIODATA::AUDIODATA(PCM_source * source)
{
	setSource(source);
}

AUDIODATA::AUDIODATA(const File & file)
{
	setSource(file);
}

void AUDIODATA::setSource(PCM_source * source)
{
	file = source->GetFileName();
	srate = source->GetSampleRate();
	channels = source->GetNumChannels();
	length = source->GetLength();
	samples = int(source->GetLength() * channels * srate);
	bitdepth = source->GetBitsPerSample();
	frames = source->GetLength() * srate;
}

void AUDIODATA::setSource(const File & file)
{
	setSource(PCM_Source_CreateFromFile(file.getFullPathName().toRawUTF8()));
}

void AUDIODATA::setSource(const vector<vector<double>> multichannelAudio, int sampleRate, int bitDepth)
{
	if (sampleRate == 0)
		return; // take is not audio

	data = multichannelAudio;
	srate = sampleRate;
	bitdepth = bitDepth;
	channels = data.size();
	length = data[0].size() / sampleRate;
	frames = data[0].size();
	samples = int(frames * channels * srate);
}

void AUDIODATA::writeToFile(const File& file) const
{
	AudioSampleBuffer buffer = convertToAudioSampleBuffer();

	WavAudioFormat format;
	std::unique_ptr<AudioFormatWriter> writer;
	writer.reset(format.createWriterFor(new FileOutputStream(file), srate, buffer.getNumChannels(), bitdepth, {}, 0));
	if (writer != nullptr)
		writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
}

void AUDIODATA::collectCues()
{
	cues = WavAudioFile::create(file, 0.0, samples / (double)srate);
}

Array<WavAudioFile::CuePoint> AUDIODATA::getCuePoints()
{
	return cues->cuePoints;
}

Array<WavAudioFile::Region> AUDIODATA::getCueRegions()
{
	return cues->regions;
}

Array<WavAudioFile::Loop> AUDIODATA::getLoops()
{
	return cues->loops;
}

void AUDIODATA::writeCues()
{
	cues->saveChanges(file);
}

void AUDIODATA::setSampleRate(int v) { srate = v; }

void AUDIODATA::setBitDepth(int v) { bitdepth = v; }

void AUDIODATA::setNumFrames(int v)
{
	for (auto & channel : data)
		channel.resize(v, 0);
}

void AUDIODATA::setNumChannels(int v)
{
	data.resize(v, vector<double>(data.size(), 0.0));
}

void AUDIODATA::setSample(int channel, int frame, double value)
{
	data[channel][frame] = value;
}

AudioSampleBuffer AUDIODATA::convertToAudioSampleBuffer() const
{
	vector<vector<float>> audio = convertAudioType<float>(data);

	AudioSampleBuffer buffer(getNumChannels(), getNumFrames());
	for (int ch = 0; ch < getNumChannels(); ++ch)
		buffer.addFrom(ch, 0, audio[ch].data(), audio[ch].size());

	return std::move(buffer);
}

//double GetNextItemStartTime(double time, TRACKLIST & TrackListIn)
//{
//    return 0.0;
//}
//
//double GetNearestItemStartTime(double time, TRACKLIST & TrackListIn)
//{
//    ITEMLIST il;
//
//    for (auto& track : TrackListIn)
//    {
//        if (track.selectedItemList().empty()) continue;
//        il.push_back(GetNearestItemToTimeBasedOnStart(time, track.selectedItemList()));
//    }
//
//    return GetNearestItemToTimeBasedOnStart(time, il).start();
//}
//
//double GetNearestItemEndTime(double time, TRACKLIST & TrackListIn)
//{
//    ITEMLIST il;
//
//    for (auto& track : TrackListIn)
//    {
//        if (track.selectedItemList().empty()) continue;
//        il.push_back(GetNearestItemToTimeBasedOnEnd(time, track.selectedItemList()));
//    }
//
//    return GetNearestItemToTimeBasedOnEnd(time, il).end();
//}
//
//ITEM GetNearestItemToTimeBasedOnStart(double time, ITEMLIST & ItemListIn)
//{
//    if (ItemListIn.size() == 1)
//        return ItemListIn.back();
//
//    int i = 0;
//    for (; i < ItemListIn.size(); ++i)
//    {
//        if (ItemListIn[i].start() > time)
//            break;
//    }
//
//    if (i == 0) 
//        return ItemListIn[0];
//
//    if (i == ItemListIn.size()) 
//        return ItemListIn.back();
//
//    ITEM & Item1 = ItemListIn[i];
//    ITEM & Item2 = ItemListIn[i-1];
//
//    if (abs(Item1.start() - time) < abs(Item2.start() - time))
//        return Item1;
//    else
//        return Item2;
//}
//
//ITEM GetNextItemToTimeBasedOnStart(double time, ITEMLIST & ItemList, int starting_idx)
//{
//    int i = starting_idx;
//    ITEM item;    
//    for (; i < ItemList.size(); ++i)
//    {
//        item = ItemList[i];
//        if (item.start() > time) break;
//    }
//    return item;
//}
//
//ITEM GetNearestItemToTimeBasedOnEnd(double time, ITEMLIST & ItemListIn)
//{
//    if (ItemListIn.size() == 1)
//        return ItemListIn.back();
//
//    int i = 0;
//    for (; i < ItemListIn.size(); ++i)
//    {
//        if (ItemListIn[i].end() > time)
//            break;
//    }
//
//    if (i == 0)
//        return ItemListIn[0];
//
//    if (i == ItemListIn.size())
//        return ItemListIn.back();
//
//    ITEM & Item1 = ItemListIn[i];
//    ITEM & Item2 = ItemListIn[i-1];
//
//    if (abs(Item1.end() - time) < abs(Item2.end() - time))
//        return Item1;
//    else
//        return Item2;
//}
//
//RANGE GetRangeOfSelectedItems(const TRACKLIST & TrackListIn)
//{
//    if (TrackListIn.empty() || TrackListIn[0].empty())
//        return RANGE();
//
//    RANGE range = TrackListIn[0][0].start();
//
//    for (const auto& Track : TrackListIn)
//        for (const auto& item : Track.selectedItemsList())
//            range.expand(item.start(), item.end());
//
//    return range;
//}
//
//ITEMGROUPLIST GetGroupsOfItems(const vector<RANGE> & list, TRACKLIST & TrackListIn)
//{
//    ITEMGROUPLIST ItemGroupList;
//
//    // go through each trange
//    for (const auto& range : list)
//    {
//        ITEMLIST ItemList;
//
//        // go through each track
//        for (int track_idx = 0; track_idx < TrackListIn.size(); ++track_idx)
//        {
//            TRACK & track = TrackListIn[track_idx];
//
//            // skip track if no selected items
//            if (!track.has_selected_items()) continue;
//
//            // go to first item on track in range
//            int num_sel_items_on_track = track.countSelectedItems();
//            int item_idx;
//            for (item_idx = 0; item_idx < num_sel_items_on_track; ++item_idx)
//                if (track.getSelectedItem(item_idx).start() >= range.start())
//                    break;
//
//            // go through each item and append item until out of range
//            for (item_idx; item_idx < num_sel_items_on_track; ++item_idx)
//            {
//                auto item = track.getSelectedItem(item_idx);
//                if (item.start() < range.end())
//                    ItemList.push_back(item);
//                else
//                    break;
//            }
//        }
//
//        // after appending all qualifying items to list, append list to group list
//        ItemGroupList.push_back(ItemList);
//    }
//    return std::move(ItemGroupList);
//}
//
//MARKERLIST GetMarkersInRange(RANGE range, MARKERLIST & MarkerListIn,  bool createGhostMarkers)
//{
//    MARKERLIST ml;
//
//    if (MarkerListIn.empty())
//        return MARKERLIST();
//
//    // if first marker is not at range start, create ghost marker if wanted
//    if (createGhostMarkers && MarkerListIn[0].start() > range.start())
//        ml.push_back(MARKER::createGhost(range.start()));
//
//    // get markers of list inside range
//    for (int i = 0; i < MarkerListIn.size(); ++i)
//    {
//        MARKER & m = MarkerListIn[i];
//        if (m.start() >= range.start() && m.start() <= range.end())
//            ml.push_back(m);
//        if (m.start() > range.end())
//            break;
//    }
//    
//    if (createGhostMarkers)
//    {
//        // if no markers found in range, create ghost marker at range start
//        if (ml.empty())
//            ml.push_back(MARKER::createGhost(range.start()));
//
//        // if no marker found at range end, create ghost marker
//        if (ml.back().start() < range.end())
//            ml.push_back(MARKER::createGhost(range.end()));
//    }
//
//    return std::move(ml);
//}
//
//MARKER GetNextMarkerOrProjectEnd(double time, MARKERLIST & MarkerListIn, int starting_idx)
//{
//    int i = starting_idx;
//    MARKER & marker = MarkerListIn[i];
//    for (; i < MarkerListIn.size(); ++i)
//    {
//        marker = MarkerListIn[i];
//        if (marker.start() > time) break;
//    }
//    return marker;
//}

//String GetPropertyStringFromKey(const String & key, bool use_value) const { return String(); }

