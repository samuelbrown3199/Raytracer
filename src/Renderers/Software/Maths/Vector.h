#ifndef VECTOR_H
#define VECTOR_H

#include <iostream>
#include <cmath>

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

inline Vector3 UnitVector(const Vector3& v) {
	return v / v.Length();
}

using Colour3 = Vector3;

inline Colour3 Get255Color(const Colour3& c) {
	return Colour3(
		255.999 * c.x(),
		255.999 * c.y(),
		255.999 * c.z()
	);
}

#endif