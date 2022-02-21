#pragma once

#undef min
#undef max

using std::min;
using std::max;

class MARKER : public OBJECT_MOVABLE, public OBJECT_NAMABLE, public OBJECT_VALIDATES
{
public:
	friend class MARKERLIST;
	static MARKER addToProject(double start, String name = "", int id = -1)
	{
		int newId = AddProjectMarker(0, false, start, start, name.toRawUTF8(), id);
		return idToIndex(newId, false);
	}
	static MARKER addToProject(double start, double end, String name = "", int id = -1)
	{
		jassert(start <= end);
		bool doCreateRegion = start < end;
		int newId = AddProjectMarker(0, doCreateRegion, start, end, name.toRawUTF8(), id);
		return idToIndex(newId, doCreateRegion);
	}
	static MARKER addToProject(const RANGE & range, const String name = "", int id = -1)
	{
		return addToProject(range.start(), range.end(), name, id);
	}
	static MARKER addToProject(const MARKER & m)
	{
		return addToProject(m.getStart(), m.getEnd(), m.getName(), m.getId());
	}
	static MARKER get(int index) { return (MARKER)index; }
	static MARKER createGhost(const RANGE & r)
	{
		return createGhost(r.start(), r.end());
	}

	// A ghost marker is one that is not yet added to the project
	static MARKER createGhost(double start, double end, const String name = "")
	{
		MARKER m;
		m._start = start;
		m._end = end;
		m.isRegion = m._start < m._end;
		m.is_ghost = true;
		m._name = name;
		m.TagManager.setString(name);
		return m;
	}

	static MARKER getRegionAtTime(double time);

	static MARKER getRegionTouchingRange(double start, double end);

	double getStart() const override { return _start; }
	void setStart(double v) override { _start = v; _set(); cache_end(); }

	double getEnd() const override { return _end; }
	void setEnd(double v) override { _end = v; if (_start != _end) isRegion = true; _set(); }

	double getLength() const override { return _end - _start; }
	void setLength(double v)  override { _end = _start + v; _set(); }

	void setPosition(double v) override { _start = v; _end = getLength() + _start; _set(); }

	Colour getColor() const override { return _color; }
	void setColor(Colour v) override { _color = v; _set(); }

protected:
	int index = -1;
	bool isRegion;
	int id = -1;
	double _start;
	double _end;
	String _name;
	Colour _color;
	void cache_end() { if (!isRegion) _end = _start; }
	bool is_ghost = false; // A ghost marker is one that is not yet added to the project

	void _set()
	{
		if (!is_ghost)
			SetProjectMarkerByIndex(0, index, isRegion, _start, _end, id, _name.toRawUTF8(), juceToReaperColor(_color));
	}
	void _get()
	{
		const char* c;
		int color;
		int en = EnumProjectMarkers3(0, index, &isRegion, &_start, &_end, &c, &id, &color);
		if (en <= 0)
		{
			index = -1;
			return;
		}
		_name = c;

		if (color <= 0)
			color = max(0, GetThemeColor(isRegion ? "region" : "marker", 0));

		_color = reaperToJuceColor(color);
		cache_end();
	}

public:
	MARKER();
	MARKER(int index);
	MARKER(RANGE range, const String & name = "");
	MARKER(double position, const String & name = "");
	MARKER(double start, double end, const String & name = "");

	// getter
	int getIndex() const { return index; }
	int getId() const { return id; }

	bool getIsRegion() const { return isRegion; }
	bool getIsMarker() const { return !isRegion; }

	// setter
	void remove() { DeleteProjectMarkerByIndex(0, index); index = -1; }

protected:
	String getObjectName() const override { return _name; }
	void setObjectName(const String & v) override { _name = v; _set(); }

	enum
	{
		__name,
		__tags,
	};

	map<String, int> method_lookup = {
			{ "N", __name },
			{ "t", __tags },
	};

	String GetPropertyStringFromKey(const String & key, bool get_value) const override;

	static int idToIndex(int lookForId, bool lookForRegion)
	{
		int idOut;
		bool isRegionOut;
		int numMarkersRegions = PROJECT::countMakersAndRegions();
		for (int i = numMarkersRegions-1; i > -1; --i)
		{
			EnumProjectMarkers3(0, i, &isRegionOut, nullptr, nullptr, nullptr, &idOut, nullptr);
			if (lookForRegion && !isRegionOut)
				continue;
			if (lookForId == idOut)
				return i;
		}

		return -1;
	}
};

class MARKERLIST : public LIST<MARKER>
{
public:
	static int CountMarkersInProject();
	static int CountRegionsInProject();
	static int CountMarkersAndRegionsInProject();

	// constructor
	MARKERLIST() {}

	// operators
	const MARKER& operator[](size_t i) const { return list[i]; }
	MARKER& operator[](size_t i) { return list[i]; }

	// conversion
	operator vector<MARKER>() { return list; }

	// collect
	void CollectMarkersAndRegions();
	void CollectMarkers();
	void CollectRegions();
	void RemoveAllFromProject();
	void AddAllToProject();
	void RemoveDuplicates();

protected:
	int num_markers = 0;
	int num_regions = 0;
	int num_markersregions = 0;

	bool has_markers = false;
	bool has_regions = false;
};

class TIMESELECTION
{
public:
	static RANGE getRange()
	{
		double timesel_start, timesel_end;
		GetSet_LoopTimeRange(false, false, &timesel_start, &timesel_end, false);
		return { timesel_start, timesel_end };
	}

	static double getStart()
	{
		double timesel_start, timesel_end;
		GetSet_LoopTimeRange(false, false, &timesel_start, &timesel_end, false);
		return timesel_start;
	}
	static double getEnd()
	{
		double timesel_start, timesel_end;
		GetSet_LoopTimeRange(false, false, &timesel_start, &timesel_end, false);
		return timesel_end;
	}
	static void setRange(const RANGE & v)
	{
		double timesel_start = v.start(), timesel_end = v.end();
		GetSet_LoopTimeRange(true, false, &timesel_start, &timesel_end, false);
	}
	static void setStart(double v)
	{
		double timesel_start, timesel_end;
		GetSet_LoopTimeRange(false, false, &timesel_start, &timesel_end, false);
		timesel_start = v;
		GetSet_LoopTimeRange(true, false, &timesel_start, &timesel_end, false);
	}

	static void setEnd(double v)
	{
		double timesel_start, timesel_end;
		GetSet_LoopTimeRange(false, false, &timesel_start, &timesel_end, false);
		timesel_end = v;
		GetSet_LoopTimeRange(true, false, &timesel_start, &timesel_end, false);
	}
};
