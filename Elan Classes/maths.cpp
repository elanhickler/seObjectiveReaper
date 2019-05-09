#include "maths.h"

bool isEqual(double a, double b, double accuracy)
{
	return abs(a - b) < accuracy;
}

bool isNotEqual(double a, double b, double accuracy)
{
	return abs(a - b) >= accuracy;
}
bool isLessThan(double a, double b, double accuracy)
{
	return abs(a - b) >= accuracy && a < b;
}
bool isMoreThan(double a, double b, double accuracy)
{
	return abs(a - b) >= accuracy && a > b;
}
bool isLessThanOrEqual(double a, double b, double accuracy)
{
	double absDelta = abs(a - b);
	return (absDelta < accuracy || absDelta >= accuracy) && a < b;
}
bool isMoreThanOrEqual(double a, double b, double accuracy)
{
	double absDelta = abs(a - b);
	return (absDelta < accuracy || absDelta >= accuracy) && a > b;
}

bool isTouchingRange(double time, double start, double end, double accuracy)
{
	return isLessThanOrEqual(end, time, accuracy) && isMoreThanOrEqual(start, time, accuracy);
}

bool isInsideRange(double time, double start, double end, double accuracy)
{
	return isLessThan(end, time, accuracy) && isMoreThan(start, time, accuracy);
}
