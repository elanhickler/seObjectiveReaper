#include "ReaperClassesHeader.h"

#include "Env.h"

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
        auto x1 = points[0].position;
        auto y1 = points[0].value;
        auto x2 = points[points.size() - 1].position;
        auto y2 = points[points.size() - 1].value;

        for (auto i = 1; i < points.size() - 1; ++i)
        {
            auto distance = getDistanceFromLine(x1, y1, x2, y2, points[i].position, points[i].value);

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
    for (const auto & p : list)
        xm += p.position;
    xm /= N;

    // y mean
    double ym = 0;
    for (const auto & p : list)
        ym += p.value;
    ym /= N;

    // x of squares
    double xx = 0;
    for (const auto & p : list)
        xx += p.position * p.position;

    // x y sum of products
    double xy = 0;
    for (int n = 0; n < N; n++)
        xy += list[n].position*list[n].value;

    a = (xy - N*xm*ym) / (xx - N*xm*xm);
    b = ym - a*xm;
}

void ENVELOPE::collectPoints()
{
    ENVPT p;
    p.id = 0;
    while (GetEnvelopePoint(envelope, p.id++, &p.position, &p.value, &p.shape, &p.tension, &p.selected))
        list.push_back(p);
}

void ENVELOPE::removeAllPoints()
{
  double start = 0;
  GetEnvelopePoint(envelope, 0, &start, nullptr, nullptr, nullptr, nullptr);
  DeleteEnvelopePointRange(envelope, start, DBL_MAX);
}

void ENVELOPE::collectAutoItemPoints(int autoitemidx)
{
    ENVPT p;
    p.id = 0;
    while (GetEnvelopePointEx(envelope, autoitemidx, p.id++, &p.position, &p.value, &p.shape, &p.tension, &p.selected))
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
        double sw  = wgt;
        double swv = wgt * list[n].value;
        k = n-1;
        while (k >= 0 && (dist = list[n].position-list[k].position) <= w2)
        { // left side loop
            wgt  = tent(dist * w2r);
            sw  += wgt;
            swv += wgt * list[k].value;
            k--;
        }
        k = n+1;
        while (k < N  && (dist = list[k].position-list[n].position) <= w2)
        { // right side loop
            wgt  = tent(dist * w2r);
            sw  += wgt;
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

void ENVELOPE::setTrackEnvelope(MediaItem_Take * take, String name) 
{
  _take = take; 
  _name = name; 
  envelope = GetTakeEnvelopeByName(_take, _name.toRawUTF8());
}

void ENVELOPE::setPoints(const ENVELOPE & env)
{
  if (!is_valid())
    envelope = ToggleTakeEnvelopeByName(_take, _name.toStdString(), true);
  removeAllPoints();
  list = env.list;
  for (const auto & pt : list)
    InsertEnvelopePoint(envelope, pt.position, pt.value, pt.shape, pt.tension, pt.selected, &no_sort);
  Envelope_SortPoints(envelope);
}
