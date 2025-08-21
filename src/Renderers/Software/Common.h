#ifndef COMMON_H
#define COMMON_H

#include <cmath>
#include <cstdlib>
#include <random>
#include <iostream>
#include <limits>
#include <memory>
#include <chrono>

const double infinity = std::numeric_limits<double>::infinity();
const double pi = 3.14159265358979323846;

inline double DegreesToRadians(double degrees)
{
	return degrees * pi / 180.0;
}

inline double RandomDouble()
{
    static std::uniform_real_distribution<double> distribution(0.0, 1.0);
    static std::mt19937 generator;
    return distribution(generator);
}

inline double RandomDouble(double min, double max)
{
	return min + (max - min) * RandomDouble();
}

inline double LinearToGamma(double linearComponent)
{
    if(linearComponent > 0)
		return std::sqrt(linearComponent);

    return 0;
}

#include "Maths/Vector.h"
#include "Maths/Interval.h"
#include "Maths/Ray.h"
#include "Hittable.h"

#endif