#ifndef COMMON_H
#define COMMON_H

#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <chrono>

const double infinity = std::numeric_limits<double>::infinity();
const double pi = 3.14159265358979323846;

inline double degrees_to_radians(double degrees)
{
	return degrees * pi / 180.0;
}

#include "Maths/Vector.h"
#include "Maths/Interval.h"
#include "Maths/Ray.h"

#endif