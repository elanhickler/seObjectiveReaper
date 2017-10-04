#pragma once

#undef min
#undef max

using std::min;
using std::max;

class MARKER : public OBJECT_MOVABLE, public OBJECT_NAMABLE, public OBJECT_VALIDATES
{
public:
    friend class MARKERLIST;
    // create a marker if only specifying position, or region if specifying start and end
    static MARKER AddToProject(double start, String name = "", int id = -1)
    {
        return AddProjectMarker(0, false, start, start, name.toRawUTF8(), id);
    }
    static MARKER AddToProject(double start, double end, String name = "", int id = -1)
    {
        return AddProjectMarker(0, start < end, start, end, name.toRawUTF8(), id);
    }
    static MARKER AddToProject(const RANGE & range, const String name = "", int id = -1)
    {
        return AddToProject(range.start(), range.end(), name, id);
    }
    static MARKER AddToProject(const MARKER & m)
    {
        return AddToProject(m.startPos(), m.endPos(), m.name(), m.id());
    }
    static MARKER get(int idx) { return std::move(MARKER(idx)); }
    static MARKER createGhost(const RANGE & r)
    {
        return createGhost(r.start(), r.end());
    }
    static MARKER createGhost(double start, double end, const String name = "")
    {
        MARKER m;
        m._start = start;
        m._end = end;
        m._is_region = m._start < m._end;
        m.is_ghost = true;
        m._name = name;
        m.TagManager.WithTags(name);
        return m;
    }
private:
    int _idx = -1;
    bool _is_region;
    int _id = -1;
    double _start;
    double _end;
    String _name;
    int _color = -1;
    void cache_end() { if (!_is_region) _end = _start; }
    bool is_ghost = false;

    void _set()
    {
        if (!is_ghost) SetProjectMarkerByIndex(0, _idx, _is_region, _start, _end, _id, _name.toRawUTF8(), _color);
    }
    void _get()
    {
        const char * c;
        if (EnumProjectMarkers3(0, _idx, &_is_region, &_start, &_end, &c, &_id, &_color) <= 0)
        {
            _idx = -1;
            return;
        }
        _name = c;
        cache_end();
    }
    String getObjectName() const override { return _name; }
    void setObjectName(const String & v) override { _name = v; _set(); }

    double getObjectStartPos() const override { return _start; }
    void setObjectStartPos(double v) override { _start = v; _set(); cache_end(); }

    double getObjectEndPos() const override { return _end; }
    void setObjectEndPos(double v) override { _end = v; if (_start != _end) _is_region = true; _set(); }

    double getObjectLength() const override { return _end - _start; }
    void setObjectLength(double v)  override { _end = _start + v; _set(); }

    void setObjectPosition(double v) override { _start = v; _end = length() + _start; _set(); }

    int getObjectColor() const override { return _color; }
    void setObjectColor(int v) override { _color = v; _set(); }

public:
    MARKER();
    MARKER(int i);
    MARKER(RANGE range, const String & name = "");
    MARKER(double position, const String & name = "");
    MARKER(double start, double end, const String & name = "");

    // getter
    int idx() const { return _idx; }
    int id() const { return _id; }

    bool is_region() const { return _is_region; }
    bool is_marker() const { return !_is_region; }

    // setter
    void remove() { DeleteProjectMarkerByIndex(0, _idx); _idx = -1; }

private:
    enum
    {
        __name,
        __tags,
    };

    map<String, int> method_lookup ={
        { "N", __name },
        { "t", __tags },
    };

    String GetPropertyStringFromKey(const String & key, bool get_value) const;
};

class MARKERLIST : public LIST<MARKER>
{
private:
    int num_markers = 0;
    int num_regions = 0;
    int num_markersregions = 0;

    MARKER invalid_marker;

    bool has_markers = false;
    bool has_regions = false;

public:
    // operators
    const MARKER& operator[](size_t i) const { return list[i]; }
    MARKER& operator[](size_t i) { return list [i]; }

    // conversion
    operator vector<MARKER>() { return list; }

    // constructor
    MARKERLIST() {}

    // collect
    int CountMarkersInProject();
    int CountRegionsInProject();
    int CountMarkersAndRegionsInProject();
    void CollectMarkersAndRegions();
    void CollectMarkers();
    void CollectRegions();
    void RemoveAllFromProject();
    void AddAllToProject();
    void RemoveDuplicates();
};

class TIMESELECTION
{
public:
    static RANGE range()
    {
        double timesel_start, timesel_end;
        GetSet_LoopTimeRange(false, false, &timesel_start, &timesel_end, false);
        return { timesel_start, timesel_end };
    }

    static double start()
    {
        double timesel_start, timesel_end;
        GetSet_LoopTimeRange(false, false, &timesel_start, &timesel_end, false);
        return timesel_start;
    }
    static double end()
    {
        double timesel_start, timesel_end;
        GetSet_LoopTimeRange(false, false, &timesel_start, &timesel_end, false);
        return timesel_end;
    }
    static void range(const RANGE & v)
    {
        double timesel_start = v.start(), timesel_end = v.end();
        GetSet_LoopTimeRange(true, false, &timesel_start, &timesel_end, false);
    }
    static void start(double v)
    {
        double timesel_start, timesel_end;
        GetSet_LoopTimeRange(false, false, &timesel_start, &timesel_end, false);
        timesel_start = v;
        GetSet_LoopTimeRange(true, false, &timesel_start, &timesel_end, false);
    }

    static void end(double v)
    {
        double timesel_start, timesel_end;
        GetSet_LoopTimeRange(false, false, &timesel_start, &timesel_end, false);
        timesel_end = v;
        GetSet_LoopTimeRange(true, false, &timesel_start, &timesel_end, false);
    }
};