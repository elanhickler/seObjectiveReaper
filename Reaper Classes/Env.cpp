#include "ReaperClassesHeader.h"

#include "Env.h"

map<String, ENVELOPE::TakeEnvMapStruct> ENVELOPE::TakeEnvMap =
{
	{ "Mute",{ (regex)"<MUTEENV\n", (regex)"<MUTEENV\n[\\s\\S]*?>\\s*", "<MUTEENV\nACT 1\nVIS 1 1 1\nLANEHEIGHT 0 0\nARM 1\nDEFSHAPE 1 -1 -1\nPT 0 1 1\n>\n" } },
	{ "Pitch",{ (regex)"<PITCHENV\n", (regex)"<PITCHENV\n[\\s\\S]*?>\\s*", "<PITCHENV\nACT 1\nVIS 1 1 1\nLANEHEIGHT 1 1\nARM 1\nDEFSHAPE 0 -1 -1\nPT 0 0 0\n>\n" } },
	{ "Pan",{ (regex)"><PANENV\n", (regex)"<PANENV\n[\\s\\S]*?>\\s*", "<PANENV\nACT 1\nVIS 1 1 1\nLANEHEIGHT 0 0\nARM 1\nDEFSHAPE 0 -1 -1\nPT 0 0 0\n>\n" } },
	{ "Volume",{ (regex)"<VOLENV\n", (regex)"<VOLENV\n[\\s\\S]*?>\\s*", "<VOLENV\nACT 1\nVIS 1 1 1\nLANEHEIGHT 0 0\nARM 1\nDEFSHAPE 0 -1 -1\nPT 0 1 0\n>\n" } }
};

float ENVELOPE::tent(float x)
{
	x = fabs(x);
	if (x > 1)
		return 0;
	return 1 - x;
}

double ENVELOPE::getDistanceFromLine(double x1, double y1, double x2, double y2, double xp, double yp)
{
	auto yDistance = y2 - y1;
	auto xDistance = x2 - x1;
	return std::abs((y2 - y1) * xp - (x2 - x1) * yp + x2 * y1 - y2 * x1) /
		std::sqrt(yDistance * yDistance + xDistance * xDistance);
}

ENVELOPE ENVELOPE::simplify(ENVELOPE points, double maxError)
{
	auto maxPointDistance = 0.0;
	auto maxPointIndex = -1;

	if (points.size() > 1)
	{
		auto x1 = points[0].getPosition();
		auto y1 = points[0].getValue();
		auto x2 = points[points.size() - 1].getPosition();
		auto y2 = points[points.size() - 1].getValue();

		for (auto i = 1; i < points.size() - 1; ++i)
		{
			auto distance = getDistanceFromLine(x1, y1, x2, y2, points[i].getPosition(), points[i].getValue());

			if (distance > maxPointDistance)
			{
				maxPointIndex = i;
				maxPointDistance = distance;
			}
		}
	}

	ENVELOPE result;

	if (maxPointDistance > maxError)
	{
		ENVELOPE lp;
		lp.list.insert(lp.begin(), points.begin(), points.begin() + maxPointIndex + 1);

		auto line1 = simplify(lp, maxError);

		ENVELOPE lq;
		lq.list.insert(lq.begin(), points.begin() + maxPointIndex, points.end());

		auto line2 = simplify(lq, maxError);

		result = line1;
		result.list.insert(result.end(), line2.begin() + 1, line2.end());
	}
	else
	{
		result.push_back(points.front());
		result.push_back(points.back());
	}

	return result;
}

void ENVELOPE::LinearRegression(ENVELOPE env, double & a, double & b)
{
	int N = list.size();

	// x mean
	double xm = 0;
	for (const auto& p : list)
		xm += p.position;
	xm /= N;

	// y mean
	double ym = 0;
	for (const auto& p : list)
		ym += p.value;
	ym /= N;

	// x of squares
	double xx = 0;
	for (const auto& p : list)
		xx += p.position * p.position;

	// x y sum of products
	double xy = 0;
	for (int n = 0; n < N; n++)
		xy += list[n].position*list[n].value;

	a = (xy - N * xm*ym) / (xx - N * xm*xm);
	b = ym - a * xm;
}

void ENVELOPE::splitTakeChunks(MediaItem* item, string chunk_c, string & header, string & footer, vector<string>& take_chunks, int & act_take_num)
{
	string chunk = chunk_c;

	// Split item chunk
	match_results<string::const_iterator> m;
	regex_search(chunk, m, chunk_to_sections);
	header = m[1];
	string data = m[2];
	footer = m[3];

	// Split data into take chunks
	sregex_token_iterator i(data.begin(), data.end(), separate_take_chunks, { -1, 0 });
	sregex_token_iterator j;
	act_take_num = 0;
	++i;
	while (i != j)
	{
		take_chunks.push_back(*i++);
		if (i != j) take_chunks.back() += *i++;
		if (!act_take_num) act_take_num = take_chunks.back().substr(0, 8) == "TAKE SEL" ? take_chunks.size() - 1 : 0;
	}
}

void ENVELOPE::toggleTakeEnvelope(MediaItem_Take * take, String env_name, bool off_on)
{
	bool update_chunk = false;
	bool env_is_enabled = false;
	string header, footer, chunk;
	vector<string> take_chunks;
	int act_take_num;
	int take_num = GetMediaItemTakeInfo_Value(take, "IP_TAKENUMBER");
	auto item = (MediaItem*)GetSetMediaItemTakeInfo(take, "P_ITEM", 0);

	char* chunk_c = GetSetObjectState(item, "");
	splitTakeChunks(item, chunk_c, header, footer, take_chunks, act_take_num);

	auto idx = TakeEnvMap[env_name];

	if (regex_search(take_chunks[take_num], idx.r_search))
		env_is_enabled = true;

	if (off_on && !env_is_enabled)
	{
		take_chunks[take_num] += idx.defchunk;
		update_chunk = true;
	}
	else if (!off_on && env_is_enabled)
	{
		take_chunks[take_num] = regex_replace(take_chunks[take_num], idx.r_replace, "$1");
		update_chunk = true;
	}

	// Rebuild item chunk
	if (update_chunk)
	{
		chunk = header;
		for (const auto& c : take_chunks) chunk += c;
		chunk += footer;

		GetSetObjectState(item, chunk.c_str());

		FreeHeapPtr(chunk_c);
	}
}

void ENVELOPE::collectPoints()
{
	ENVPT p;
	p.index = 0;
	while (GetEnvelopePoint(envelopePtr, p.index, &p.position, &p.value, &p.shape, &p.tension, &p.selected))
	{
		list.push_back(p);
		++p.index;
	}
}

void ENVELOPE::removeAllPoints()
{
	double start = 0;
	GetEnvelopePoint(envelopePtr, 0, &start, nullptr, nullptr, nullptr, nullptr);
	DeleteEnvelopePointRange(envelopePtr, start, DBL_MAX);
}

void ENVELOPE::collectAutoItemPoints(int autoitemidx)
{
	ENVPT p;
	p.index = 0;
	while (GetEnvelopePointEx(envelopePtr, autoitemidx, p.index++, &p.position, &p.value, &p.shape, &p.tension, &p.selected))
		list.push_back(p);
}

void ENVELOPE::simplifyByAverage(double width)
{
	ENVELOPE env;
	double w2 = width;
	double w2r = 1 / w2;
	double dist;
	int k;
	int N = list.size();
	for (int n = 0; n < list.size(); n++)
	{
		double wgt = tent(0);
		double sw = wgt;
		double swv = wgt * list[n].value;
		k = n - 1;
		while (k >= 0 && (dist = list[n].position - list[k].position) <= w2)
		{ // left side loop
			wgt = tent(dist * w2r);
			sw += wgt;
			swv += wgt * list[k].value;
			k--;
		}
		k = n + 1;
		while (k < N && (dist = list[k].position - list[n].position) <= w2)
		{ // right side loop
			wgt = tent(dist * w2r);
			sw += wgt;
			swv += wgt * list[k].value;
			k++;
		}
		env.push_back({ list[n].position, swv / sw, list[n].shape, list[n].tension, list[n].selected });
	}

	list = env.list;
}

void ENVELOPE::simplifyByDifference(double diff)
{
	list = simplify(*this, diff).list;
}

double ENVELOPE::centerValueTowardAverage(double min_x, double max_x)
{
	double a, b;
	LinearRegression(*this, a, b);
	// center x-value (assuming x is ascending)
	double xc = 0.5 * (min_x + max_x);
	// center y-value based on regression line
	return a * xc + b;
}

void ENVELOPE::setEnvelope(MediaItem_Take * take, String name)
{
	takePtr = take;
	envelopePtr = GetTakeEnvelopeByName(takePtr, _name.toRawUTF8());
	_name = name;
}

void ENVELOPE::setEnvelope(TrackEnvelope* trackEnv)
{
	takePtr = nullptr;
	envelopePtr = trackEnv;
	char buf[1024];
	GetEnvelopeName(envelopePtr, buf, 1024);
	_name = String(buf);
}

void ENVELOPE::setPoints(const ENVELOPE & env)
{
	if (!isValid())
	{
		toggleByName(takePtr, _name.toStdString(), true);
		envelopePtr = ENVELOPE::getByName(takePtr, _name);
	}

	removeAllPoints();

	list = env.list;

	for (const auto& pt : list)
		InsertEnvelopePoint(envelopePtr, pt.position, pt.value, pt.shape, pt.tension, pt.selected, &no_sort);

	Envelope_SortPoints(envelopePtr);
}

void ENVELOPE::toggle(bool off_on)
{
	if (takePtr != nullptr)
		toggleTakeEnvelope(takePtr, _name, off_on);
}

void ENVELOPE::toggleByName(MediaItem_Take * take, string env_name, bool off_on)
{
	bool update_chunk = false;
	bool env_is_enabled = false;
	string header, footer, chunk;
	vector<string> take_chunks;
	int act_take_num;
	int take_num = GetMediaItemTakeInfo_Value(take, "IP_TAKENUMBER");
	auto item = (MediaItem*)GetSetMediaItemTakeInfo(take, "P_ITEM", 0);

	char* chunk_c = GetSetObjectState(item, "");
	splitTakeChunks(item, chunk_c, header, footer, take_chunks, act_take_num);

	auto idx = TakeEnvMap[env_name];

	if (regex_search(take_chunks[take_num], idx.r_search)) env_is_enabled = true;

	if (off_on && !env_is_enabled)
	{
		take_chunks[take_num] += idx.defchunk;
		update_chunk = true;
	}
	else if (!off_on && env_is_enabled)
	{
		take_chunks[take_num] = regex_replace(take_chunks[take_num], idx.r_replace, "$1");
		update_chunk = true;
	}

	// Rebuild item chunk
	if (update_chunk)
	{
		chunk = header;
		for (const auto& c : take_chunks) chunk += c;
		chunk += footer;

		GetSetObjectState(item, chunk.c_str());

		FreeHeapPtr(chunk_c);
	}
}

TrackEnvelope * ENVELOPE::getByName(MediaItem_Take * take, String name)
{
	return GetTakeEnvelopeByName(take, name.toUTF8());
}

ENVELOPE::ENVELOPE(TrackEnvelope * envelopePtr) : envelopePtr(envelopePtr)
{
	char buf[1024];
	GetEnvelopeName(envelopePtr, buf, 1024);
	_name = String(buf);
}

ENVELOPE::ENVELOPE(TrackEnvelope * envelopePtr, String name) : envelopePtr(envelopePtr), _name(name) {}

ENVELOPE::ENVELOPE(MediaItem_Take * take, String name)
{
	takePtr = take;
	_name = name;
	envelopePtr = GetTakeEnvelopeByName(takePtr, _name.toRawUTF8());
}
