#ifndef INTERVAL_H
#define INTERVAL_H

class Interval
{
public:

	double min, max;
	Interval() : min(+infinity), max(-infinity) {}

	Interval(double min, double max) : min(min), max(max) {}

	double Size() const 
	{
		return max - min;
	}

	bool Contains(double x) const
	{
		return x >= min && x <= max;
	}

	bool Surrounds(double x) const
	{
		return min < x && x < max;
	}

	static const Interval Empty, Universe;
};

#endif