#include "ReaperClassesHeader.h"
#include "Marker.h"

int MARKERLIST::CountMarkersInProject()
{
	int m;
	CountProjectMarkers(nullptr, &m, nullptr);
	return m;
}

int MARKERLIST::CountRegionsInProject()
{
	int r;
	CountProjectMarkers(nullptr, nullptr, &r);
	return r;
}

int MARKERLIST::CountMarkersAndRegionsInProject()
{
	int m, r;
	CountProjectMarkers(nullptr, &m, &r);
	return m + r;
}

void MARKERLIST::CollectMarkersAndRegions()
{
	int count = CountMarkersAndRegionsInProject();

	for (int i = 0; i < count; ++i)
		list.push_back(MARKER(i));
}

void MARKERLIST::CollectMarkers()
{
	int count = CountMarkersAndRegionsInProject();

	for (int i = 0; i < count; ++i)
		if (MARKER(i).getIsMarker())
			list.push_back(MARKER(i));
}

void MARKERLIST::CollectRegions()
{
	int count = CountMarkersAndRegionsInProject();

	for (int i = 0; i < count; ++i)
		if (MARKER(i).getIsRegion())
			list.push_back(MARKER(i));
}

void MARKERLIST::RemoveAllFromProject()
{
	int deletions = 0;
	for (auto& m : list)
		DeleteProjectMarkerByIndex(0, m.getIndex() - deletions++);
}

void MARKERLIST::AddAllToProject()
{
	for (const auto& m : list)
		MARKER::addToProject(m);
}

void MARKERLIST::RemoveDuplicates()
{
	if (size() < 2) return;
	sort();
	MARKER * m;
	for (int i = 0; i < size();)
	{
		m = &list[i];
		while (++i < size() && m->range() == list[i].range())
			list[i].makeInvalid();
	}
	MARKERLIST NewMarkerList;
	for (const auto& mr : list)
		if (mr.isValid())
			NewMarkerList.push_back(mr);
	*this = std::move(NewMarkerList);
}

MARKER::MARKER() {}
MARKER::MARKER(int index) : index(index)
{
	_get();
	cache_end();
	TagManager.setStringWithTags(getName());
}

MARKER::MARKER(RANGE range, const String & name)
{
	_start = range.start();
	_end = range.end();
	isRegion = _start < _end;
	is_ghost = true;
	_name = name;
	TagManager.setStringWithTags(name);
}

MARKER::MARKER(double position, const String & name)
{
	_start = position;
	_end = position;
	isRegion = false;
	is_ghost = true;
	_name = name;
	TagManager.setStringWithTags(name);
}

MARKER::MARKER(double start, double end, const String & name)
{
	_start = start;
	_end = end;
	isRegion = _start < _end;
	is_ghost = true;
	_name = name;
	TagManager.setStringWithTags(name);
}

String MARKER::GetPropertyStringFromKey(const String & key, bool get_value) const
{
	auto iter = method_lookup.find(key);

	if (iter == method_lookup.end())
		return getTag(key);

	switch (iter->second)
	{
	case __name:
		if (get_value)
			return String(getIndex());
		return getNameNoTags();
	case __tags:
		return getTagString();
	}

	return String();
}
