#pragma once

using std::min;
using std::max;

class RANGE
{
private:
  double s; //start
  double e; //end

public:
  RANGE() {};
  RANGE(double start, double end) { s = min(start, end); e = max(start, end); }
  RANGE(double time) : s(time), e(time) {}
  template <class T, class U> RANGE(const T & a, const U & b)
  {
    s = min(a.range().start(), b.range().start());
    e = max(a.range().end(), b.range().end());
  }

  inline bool is_touching(const RANGE & r) const
  {
		double a1 = s;
		double a2 = e;
		double b1 = r.start();
		double b2 = r.end();

    if (isLessThanOrEqual(a1,b1))
      return isLessThanOrEqual(a1, b1) && (isMoreThanOrEqual(a2,b2) || isMoreThanOrEqual(a2,b1));
    else
      return isMoreThanOrEqual(a1, b1) && (isLessThanOrEqual(a2,b2) || isLessThanOrEqual(a2, b1));
  }
  inline bool is_overlapping(const RANGE & r) const
  {
		double a1 = s;
		double a2 = e;
		double b1 = r.start();
		double b2 = r.end();

    if (isLessThanOrEqual(a1,b1))
      return isLessThan(a1,b1) && (isMoreThan(a2,b2) || isMoreThan(a2,b1));
    else
      return isMoreThan(a1,b1) && (isLessThan(a2,b2) || isLessThan(a2,b1));
  }
  inline bool is_inside(const RANGE & r) const
  {
    return s >= r.start()+1.e-6 && e <= r.end()-1.e-6;
  }
  inline bool is_before(const RANGE & r) const
  {
    return isLessThan(e,r.start());
  }
  inline bool is_after(const RANGE & r) const
  {
    return isMoreThan(s,r.end());
  }
  inline bool is_outside(const RANGE & r) const
  {
    return isLessThan(e,r.start()) || isMoreThan(s,r.end());
  }

  static inline bool is_touching(double a, double b) { return RANGE(a).range().is_touching(RANGE(b).range()); }
  static inline bool is_overlapping(double a, double b) { return RANGE(a).range().is_overlapping(RANGE(b).range()); }
  static inline bool is_inside(double a, double b) { return RANGE(a).range().is_inside(RANGE(b).range()); }
  static inline bool is_outside(double a, double b) { return RANGE(a).range().is_outside(RANGE(b).range()); }
  static inline bool is_before(double a, double b) { return RANGE(a).range().is_before(RANGE(b).range()); }
  static inline bool is_after(double a, double b) { return RANGE(a).range().is_after(RANGE(b).range()); }

  static inline bool is_touching(double a, const RANGE & b) { return  RANGE(a).range().is_touching(b.range()); }
  static inline bool is_overlapping(double a, const RANGE & b) { return RANGE(a).range().is_overlapping(b.range()); }
  static inline bool is_inside(double a, const RANGE & b) { return RANGE(a).range().is_inside(b.range()); }
  static inline bool is_outside(double a, const RANGE & b) { return RANGE(a).range().is_outside(b.range()); }
  static inline bool is_before(double a, const RANGE & b) { return RANGE(a).range().is_before(b.range()); }
  static inline bool is_after(double a, const RANGE & b) { return RANGE(a).range().is_after(b.range()); }

  template <class T, class U> static inline bool is_touching(const T & a, const U & b) { return a.range().is_touching(b.range()); }
  template <class T, class U> static inline bool is_overlapping(const T & a, const U & b) { return a.range().is_overlapping(b.range()); }
  template <class T, class U> static inline bool is_inside(const T & a, const U & b) { return a.range().is_inside(b.range()); }
  template <class T, class U> static inline bool is_outside(const T & a, const U & b) { return a.range().is_outside(b.range()); }
  template <class T, class U> static inline bool is_before(const T & a, const U & b) { return a.range().is_before(b.range()); }
  template <class T, class U> static inline bool is_after(const T & a, const U & b) { a.range().is_after(b.range()); }

  template <class T> static inline bool is_touching(const T & a, double b) { return a.range().is_touching(b); }
  template <class T> static inline bool is_overlapping(const T & a, double b) { return a.range().is_overlapping(b); }
  template <class T> static inline bool is_inside(const T & a, double b) { return a.range().is_inside(b); }
  template <class T> static inline bool is_outside(const T & a, double b) { return a.range().is_outside(b); }
  template <class T> static inline bool is_before(const T & a, double b) { return a.range().is_before(b); }
  template <class T> static inline bool is_after(const T & a, double b) { a.range().is_after(b); }

  // operator
  RANGE operator=(double t) { s = t; e = t; return *this; }
  RANGE operator=(RANGE r) { s = r.s; e = r.e; return *this; }
  bool operator==(const RANGE & rhs) const { return isEqual(s,rhs.s) && isEqual(e,rhs.e); }

  // getter
  inline double start() const { return s; }
  inline double length() const { return e - s; }
  inline double end() const { return e; }
  inline double pos() const { return s; }
  RANGE range() const { return { s,e }; }

  // setter

  // ensure range is at least wide as given range
  inline void expand(RANGE r) { s = min(s, r.start()); e = max(e, r.end()); }
  inline void expand(double start, double end) { expand({ start,end }); }
  inline void start(double v) { s = v; }
  inline void end(double v) { e = v; }
  inline void pos(double v) { s = v; e = e-s + v; }
  inline void range(RANGE r) { s = r.start(); e = r.end(); }
};

//double GetRandomAmplitude()
//{
//    static std::mt19937 mt(0);
//    static std::uniform_real_distribution<double> distribution(-1.0, 1.0);
//    return distribution(mt);
//}

template<class t> void flip(t & value) { value = !value; }
template<class t> bool is_even(const t & value) { return value % 2 == 0; }
template<class t> bool is_odd(const t & value) { return value % 2 != 0; }
template<class t> t clamp(t in, t min, t max)
{
  if (in <= min) return min;
  if (in >= max) return max;
  return in;
}

template<class t> t ampToDb(t a)
{
  return 20.0*log10(a);
  //return 6.0*log2(a); // faster approximation
}

template<class t> t dbToAmp(t db)
{
  return pow(10, db/20.0);
  //return pow(2, db/6.0); // faster approximation
}
