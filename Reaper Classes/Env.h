#pragma once

class ENVPT
{
public:
	enum shape { linear, square, s_curve, log, exp, bezier };
	static ENVPT getByTime(TrackEnvelope * envelope, double time)
	{
		int pt_idx = GetEnvelopePointByTime(envelope, time);
		double position, value, tension;
		bool selected;
		int shape;
		GetEnvelopePoint(envelope, pt_idx, &position, &value, &shape, &tension, &selected);
		return ENVPT(pt_idx, position, value, shape, tension, selected);
	}
public:
	// members
	int id = -1;
	double value = -1.0;
	double position = -1.0;
	int shape = ENVPT::shape::linear;
	double tension = 0.0;
	bool selected = false;

	double getStart() { return position; }

	ENVPT() {}
	ENVPT(int idx, double p, double v, int s = 0, double t = 0.0, bool sel = false) : id(idx), position(p), value(v), shape(s), tension(t), selected(sel) {}
	ENVPT(double p, double v, int s = 0, double t = 0.0, bool sel = false) : position(p), value(v), shape(s), tension(t), selected(sel) {}
};

class ENVELOPE : public LIST<ENVPT>
{
public:
	static TrackEnvelope* getSelected()
	{
		return GetSelectedEnvelope(nullptr);
	}
	static void toggleByName(MediaItem_Take* take, string env_name, bool off_on);
	static TrackEnvelope* getByName(MediaItem_Take* take, String name);

	ENVELOPE() {}
	ENVELOPE(TrackEnvelope * envelopePtr);
	ENVELOPE(TrackEnvelope * envelopePtr, String name);
	ENVELOPE(MediaItem_Take * take, String name);

	// conversion
	operator vector<ENVPT>() { return list; }
	operator TrackEnvelope*() const { return envelopePtr; }

	// operators
	bool operator==(TrackEnvelope * rhs) const { return envelopePtr == rhs; }
	bool operator!=(TrackEnvelope * rhs) const { return envelopePtr != rhs; }
	bool operator==(const ENVELOPE & rhs) const { return envelopePtr == rhs.envelopePtr; }
	bool operator!=(const ENVELOPE & rhs) const { return envelopePtr != rhs.envelopePtr; }

	// functions
	void collectPoints();
	void removeAllPoints();
	void collectAutoItemPoints(int autoitemidx);
	void simplifyByAverage(double width);
	void simplifyByDifference(double diff);
	double centerValueTowardAverage(double min_x, double max_x);

	void setEnvelope(MediaItem_Take* take, String name);
	void setEnvelope(TrackEnvelope* trackEnv);
	void setPoints(const ENVELOPE & env);

	void toggle(bool off_on);

	// boolean
	bool isValid() const { return envelopePtr != nullptr; }

protected:
	bool no_sort = true;
	float tent(float x);
	double getDistanceFromLine(double x1, double y1, double x2, double y2, double xp, double yp);
	ENVELOPE simplify(ENVELOPE points, double maxError);
	void LinearRegression(ENVELOPE p, double & a, double & b);

	// members
	String _name;
	MediaItem_Take* takePtr = nullptr;
	TrackEnvelope* envelopePtr = nullptr;

	// chunk stuff
	struct TakeEnvMapStruct { regex r_search, r_replace; string defchunk; };
	static map<String, TakeEnvMapStruct> TakeEnvMap;
	static void splitTakeChunks(MediaItem * item, string chunk_c, string& header, string& footer, vector<string>& take_chunks, int& act_take_num);
	static void toggleTakeEnvelope(MediaItem_Take * take, String env_name, bool off_on);
};

class AUTOITEM : public OBJECT_MOVABLE, public OBJECT_VALIDATES
{
public:
	static AUTOITEM create(TrackEnvelope* envelopePtr, double position, double length)
	{
		return InsertAutomationItem(envelopePtr, -1, position, length);
	}

	AUTOITEM() {}
	AUTOITEM(int idx) : _idx(idx)
	{
		_get();
		collectPoints();
	}

	void collectPoints() { _envelope.collectAutoItemPoints(_idx); }
	ENVELOPE envelope() const { return _envelope; }
	void simplifyByAverage(double width) { _envelope.simplifyByAverage(width); }
	void simplifyByDifference(double diff) { _envelope.simplifyByDifference(diff); }
	double centerValueTowardAverage() { return _envelope.centerValueTowardAverage(0, getLength()); }
	AUTOITEM create() { return InsertAutomationItem(_envelope, _pool_id, _position, _length); }

	double getStart() const override { return _position; }
	void setStart(double v) override { _position = v; _set(); }

	double getLength() const override { return _length; }
	void setLength(double v) override { _length = v; _set(); }

	double getEnd() const override { return getStart() + getLength(); }
	void setEnd(double v) override { jassertfalse; }

	void setPosition(double v) override { _position = v; _set(); }

	bool isValid() const override { return _idx != -1; }

protected:
	ENVELOPE _envelope;
	int _idx = -1;
	int _pool_id = -1;
	double _position = 0;
	double _length = 0;
	double _end = 0;
	double _startoffs = 0;
	double _playrate = 1;
	double _baseline = 0;
	double _amplitude = 100;
	double _loopsrc = 0;
	bool _sel = false;

	void _get()
	{
		if (!isValid())
			return;

		_pool_id = GetSetAutomationItemInfo(_envelope, _idx, "D_POOL_ID", 0.0, false);
		_position = GetSetAutomationItemInfo(_envelope, _idx, "D_POSITION", 0.0, false);
		_length = GetSetAutomationItemInfo(_envelope, _idx, "D_LENGTH", 0.0, false);
		_startoffs = GetSetAutomationItemInfo(_envelope, _idx, "D_STARTOFFS", 0.0, false);
		_playrate = GetSetAutomationItemInfo(_envelope, _idx, "D_PLAYRATE", 0.0, false);
		_baseline = GetSetAutomationItemInfo(_envelope, _idx, "D_BASELINE", 0.0, false);
		_amplitude = GetSetAutomationItemInfo(_envelope, _idx, "D_AMPLITUDE", 0.0, false);
		_loopsrc = GetSetAutomationItemInfo(_envelope, _idx, "D_LOOPSRC", 0.0, false);
		_sel = GetSetAutomationItemInfo(_envelope, _idx, "D_UISEL", 0.0, false) > 0;
		_cache_end();
	}

	void _set()
	{
		if (!isValid())
			return;

		GetSetAutomationItemInfo(_envelope, _idx, "D_POOL_ID", _pool_id, true);
		GetSetAutomationItemInfo(_envelope, _idx, "D_POSITION", _position, true);
		GetSetAutomationItemInfo(_envelope, _idx, "D_LENGTH", _length, true);
		GetSetAutomationItemInfo(_envelope, _idx, "D_STARTOFFS", _startoffs, true);
		GetSetAutomationItemInfo(_envelope, _idx, "D_PLAYRATE", _playrate, true);
		GetSetAutomationItemInfo(_envelope, _idx, "D_BASELINE", _baseline, true);
		GetSetAutomationItemInfo(_envelope, _idx, "D_AMPLITUDE", _amplitude, true);
		GetSetAutomationItemInfo(_envelope, _idx, "D_LOOPSRC", _loopsrc, true);
		GetSetAutomationItemInfo(_envelope, _idx, "D_UISEL", _sel ? 1.0 : 0.0, true);
	}

	void _cache_end() { _end = _position + _length; }
};

class AUTOITEMLIST : public LIST<AUTOITEM>
{
public:
	AUTOITEMLIST() {}

	void collectAutomationItems()
	{
		//for (const auto& ai : list)
		//{
		//	int num_items = CountAutomationItems(ai.envelope());
		//	for (int i = 0; i < num_items; ++i)
		//		list.push_back(AUTOITEM(i));
		//}
	}
};
