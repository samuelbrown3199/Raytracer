#ifndef RAY_H
#define RAY_H

class Ray
{
private:

	Vector3 m_origin;
	Vector3 m_direction;

public:

	Ray() {};
	Ray(const Vector3& origin, const Vector3& direction)
		: m_origin(origin), m_direction(direction) {
	}

	const Vector3& Origin() const {
		return m_origin;
	}

	const Vector3& Direction() const {
		return m_direction;
	}

	Vector3 PointAtT(float t) const {
		return m_origin + t * m_direction;
	}
};

#endif