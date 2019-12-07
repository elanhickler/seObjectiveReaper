#pragma once

class ENVPT : public OBJECT_MOVABLE, public OBJECT_VALIDATES
{
	friend class ENVELOPE;
public:
	enum shape { linear, square, s_curve, log, exp, bezier };

	static ENVPT getNearestToTime(TrackEnvelope* envelope, double time)
	{
		int index1 = GetEnvelopePointByTime(envelope, time);
		int index2 = index1 + 1;

		ENVPT p1 = getByIndex(envelope, p1.index);		
		ENVPT p2 = getByIndex(envelope, index2);

		double p1_delta = abs(p1.getPosition() - time);
		double p2_delta = abs(p2.getPosition() - time);

		return p1_delta < p2_delta ? p1 : p2;
	}

	static ENVPT getNearestBeforeTime(TrackEnvelope* envelope, double time)
	{
		int index = GetEnvelopePointByTime(envelope, time);

		ENVPT p1 = getByIndex(envelope, index);
		ENVPT p2 = getByIndex(envelope, index + 1);

		if (!p2.isValid())
			return p1;

		if (isMoreThan(p2.getPosition(), time))
		{
			if (isLessThanOrEqual(p1.getPosition(), time))
				return p1.isValid() ? p1 : ENVPT();
			else
				return ENVPT();
		}

		double p1_delta = abs(p1.getPosition() - time);
		double p2_delta = abs(p2.getPosition() - time);

		if (p1_delta < p2_delta)
				return p1;

		return p2;
	}

	template<typename Q, typename I, typename Distance>
	void find_n_nearest(const Q& q, I first, I nth, I last, Distance dist)
	{
		using T = decltype(*first);
		auto compare = [&q, &dist](T i, T j) { return dist(i, q) < dist(j, q); };
		std::nth_element(first, nth, last, compare);
		std::sort(first, last, compare);
	}

	static ENVPT getNearestAfterTime(TrackEnvelope* envelope, double time)
	{
		int startIndex = GetEnvelopePointByTime(envelope, time);

		vector<ENVPT> points;
		for (int i = startIndex; i < startIndex + 2; ++i)
		{
			auto p = getByIndex(envelope, i);
			if (p.isValid() && isMoreThanOrEqual(p.getPosition(), time))
				points.push_back(p);
		}

		if (points.size() == 0)
			return {};

		if (points.size() == 1)
			return points[0];

		double lastDelta = abs(points[0].getPosition() - time);
		int lastIndex = 0;
		for (int i = 1; i < points.size(); ++i)
		{
			ENVPT pt = points[i];
			double delta = abs(points[i].getPosition() - time);
			if (delta < lastDelta)
			{
				lastDelta = delta;
				lastIndex = i;
			}
		}

		return points[lastIndex];
	}

	static ENVPT getByIndex(TrackEnvelope* envelope, int idx)
	{
		ENVPT p;
		p.index = idx;
		GetEnvelopePoint(envelope, idx, &p.position, &p.value, &p.shape, &p.tension, &p.selected);
		return p;
	}

	ENVPT() {}
	ENVPT(int index, double position, double value, int shape = 0, double tension = 0.0, bool sel = false)
		: index(index)
		, position(position)
		, value(value)
		, shape(shape)
		, tension(tension),
		selected(sel)
	{}

	ENVPT(double position, double value, int shape = 0, double tension = 0.0, bool selected = false)
		: position(position)
		, value(value)
		, shape(shape)
		, tension(tension)
		, selected(selected)
	{}

	double getPosition() const { return position; }
	void setPosition(double v) { position = v; }

	double getStart() const override { return position; }
	double getEnd() const override { return position; }

	int getIndex() const { return index; }

	double getValue() const { return value; }
	void setValue(double v) { value = v; }

	bool isValid() const override
	{
		return position != -1.0;
	}

//protected:
	int index = -1;
	double value = -1.0;
	double position = -1.0;
	int shape = ENVPT::shape::linear;
	double tension = 0.0;
	bool selected = false;
};

class ENVELOPE : public LIST<ENVPT>, public OBJECT_VALIDATES
{
public:
	static ENVELOPE getMasterPlayRateEnvelope()
	{
		ENVELOPE e;
		e.envelopePtr = GetTrackEnvelopeByName(GetMasterTrack(0), "Playrate");
		return std::move(e);
	}

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

	// removes points based on given indexes of points inside envelope object
	void removePoints(const ENVELOPE& v)
	{
		for (int i = v.size(); i--; )
			list.erase(begin() + v[i].getIndex());
	}
	void collectAutoItemPoints(int autoitemidx);
	void simplifyByAverage(double width);
	void simplifyByDifference(double diff);
	double centerValueTowardAverage(double min_x, double max_x);

	// sets the reaper envelope to edit
	void setEnvelope(MediaItem_Take* take, String name);
	// sets the reaper envelope to edit
	void setEnvelope(TrackEnvelope* trackEnv);
	// sets given reaper envelope points based on given envelope object
	void setPoints(const ENVELOPE & env);

	ENVELOPE getPointsInRange(double start, double end)
	{
		ENVELOPE envOut;
		envOut.envelopePtr = this->envelopePtr;

		auto firstPoint = ENVPT::getNearestAfterTime(envelopePtr, start);
		auto lastPoint = ENVPT::getNearestBeforeTime(envelopePtr, end);

		if (!firstPoint.isValid())
		{
			if (lastPoint.isValid() && isMoreThanOrEqual(lastPoint.getPosition(), start))
				envOut.push_back(lastPoint);

			return envOut;
		}

		bool firstPointBeyondEnd = isMoreThanOrEqual(firstPoint.getPosition(), end);
		bool lastPointBeforeStart = isLessThanOrEqual(lastPoint.getPosition(), start);

		if (firstPointBeyondEnd || lastPointBeforeStart)
			return envOut;

		if (firstPoint.getIndex() == lastPoint.getIndex())
		{
			envOut.push_back(firstPoint);
			return envOut;
		}

		for (int i = firstPoint.getIndex(); i <= lastPoint.getIndex(); ++i)
			envOut.push_back(this->list[i]);

		return envOut;
	}

	void removePointsInRange(double start, double end)
	{
		auto envelope = getPointsInRange(start, end);

		if (envelope.empty())
			return;

		auto startIndex = envelope.front().getIndex() - 1;
		auto endIndex = envelope.back().getIndex() - 1;

		list.erase(list.begin() + startIndex, list.begin() + endIndex + 1);
	}

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
		jassertfalse;
	}
};
