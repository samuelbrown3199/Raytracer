#ifndef VECTOR_H
#define VECTOR_H

#include <iostream>
#include <cmath>

#include "Interval.h"

class Vector3
{
	double e[3];

public:

	double x() const { return e[0]; }
	double y() const { return e[1]; }
	double z() const { return e[2]; }

	Vector3() : e{ 0, 0, 0 } {}
	Vector3(double x, double y, double z) : e{ x, y, z } {}

	Vector3 operator- () const {
		return Vector3(-e[0], -e[1], -e[2]);
	}
	
	double operator[](int i) const {
		return e[i];
	}

	double& operator[](int i) {
		return e[i];
	}

	Vector3 operator+(const Vector3& v) const {
		return Vector3(e[0] + v.e[0], e[1] + v.e[1], e[2] + v.e[2]);
	}

	Vector3 operator+=(const Vector3& v) {
		e[0] += v.e[0];
		e[1] += v.e[1];
		e[2] += v.e[2];
		return *this;
	}

	Vector3 operator-(const Vector3& v) const {
		return Vector3(e[0] - v.e[0], e[1] - v.e[1], e[2] - v.e[2]);
	}

	Vector3 operator-=(const Vector3& v) {
		e[0] -= v.e[0];
		e[1] -= v.e[1];
		e[2] -= v.e[2];
		return *this;
	}

	Vector3 operator*(double t) const {
		return Vector3(e[0] * t, e[1] * t, e[2] * t);
	}

	Vector3 operator*=(double t) {
		e[0] *= t;
		e[1] *= t;
		e[2] *= t;
		return *this;
	}

	Vector3 operator*(const Vector3& v) const {
		return Vector3(e[0] * v.e[0], e[1] * v.e[1], e[2] * v.e[2]);
	}

	Vector3 operator/(double t) const {
		return Vector3(e[0] / t, e[1] / t, e[2] / t);
	}

	Vector3 operator/=(double t) {
		e[0] /= t;
		e[1] /= t;
		e[2] /= t;
		return *this;
	}

	double Length() const {
		return std::sqrt(e[0] * e[0] + e[1] * e[1] + e[2] * e[2]);
	}

	double SqrLength() const {
		return e[0] * e[0] + e[1] * e[1] + e[2] * e[2];
	}

	static Vector3 Random()
	{
		return Vector3(RandomDouble(), RandomDouble(), RandomDouble());
	}

	static Vector3 Random(double min, double max)
	{
		return Vector3(RandomDouble(min, max), RandomDouble(min, max), RandomDouble(min, max));
	}

	bool NearZero() const
	{
		const double s = 1e-8;
		return (fabs(e[0]) < s) && (fabs(e[1]) < s) && (fabs(e[2]) < s);
	}

};

inline std::ostream& operator<<(std::ostream& out, const Vector3& v) {
	return out << v[0] << ' ' << v[1] << ' ' << v[2];
}

inline Vector3 operator*(double t, const Vector3& v) {
	return Vector3(v.x() * t, v.y() * t, v.z() * t);
}

inline double Dot(const Vector3& u, const Vector3& v) {
	return u.x() * v.x() + u.y() * v.y() + u.z() * v.z();
}

inline Vector3 Cross(const Vector3& u, const Vector3& v) {
	return Vector3(
		u.y() * v.z() - u.z() * v.y(),
		u.z() * v.x() - u.x() * v.z(),
		u.x() * v.y() - u.y() * v.x()
	);
}

inline Vector3 UnitVector(const Vector3& v)
{
	return v / v.Length();
}

inline Vector3 RandomInUnitDisk()
{
	while (true)
	{
		auto p = Vector3(RandomDouble(-1, 1), RandomDouble(-1, 1), 0);
		if (p.SqrLength() < 1)
			return p;
	}
}

inline Vector3 RandomUnitVector()
{
	while (true)
	{
		auto p = Vector3::Random(-1, 1);
		auto lensq = p.SqrLength();
		if (1e-160 < lensq && lensq <= 1)
			return p / sqrt(lensq);
	}
}

inline Vector3 RandomOnHemisphere(const Vector3& normal)
{
	Vector3 onUnitSphere = RandomUnitVector();
	if (Dot(onUnitSphere, normal) > 0)
		return onUnitSphere;
	else
		return -onUnitSphere;
}

inline Vector3 Reflect(const Vector3& v, const Vector3& n)
{
	return v - 2 * Dot(v, n) * n;
}

inline Vector3 Refract(const Vector3& uv, const Vector3& n, double eta)
{
	auto cosTheta = std::fmin(Dot(-uv, n), 1.0);
	Vector3 rOutPerp = eta * (uv + cosTheta * n);
	Vector3 rOutParallel = -std::sqrt(std::fabs(1.0 - rOutPerp.SqrLength())) * n;
	return rOutPerp + rOutParallel;
}

using Colour3 = Vector3;

inline Colour3 Get255Color(const Colour3& c) {

	static const Interval intensity(0.000, 0.999);
	int rbyte = int(256 * intensity.Clamp(c.x()));
	int gbyte = int(256 * intensity.Clamp(c.y()));
	int bbyte = int(256 * intensity.Clamp(c.z()));

	return Colour3(
		rbyte,
		gbyte,
		bbyte
	);
}

#endif