#ifndef HITTABLE_H
#define HITTABLE_H

#include "Maths/AABB.h"
#include "../ModelLoader.h"

class Material;

class HitRecord
{
public:

	Vector3 p;
	Vector3 normal;
	std::shared_ptr<Material> mat;
	double t;
	double u;
	double v;
	bool frontFace;

	void SetFaceNormal(const Ray& r, const Vector3& outwardNormal)
	{
		frontFace = Dot(r.Direction(), outwardNormal) < 0;
		normal = frontFace ? outwardNormal : -outwardNormal;
	}
};

class Hittable
{
public:
	virtual ~Hittable() = default;
	virtual bool Hit(const Ray& r, Interval t, HitRecord& rec) const = 0;

	virtual AABB BoundingBox() const = 0;
};

class Sphere : public Hittable
{
public:

	Sphere(const Vector3& center, double radius, std::shared_ptr<Material> mat)
		: center(center), radius(radius), mat(mat)
	{
		auto rVec = Vector3(radius, radius, radius);
		bbox = AABB(center - rVec, center + rVec);
	}

	AABB BoundingBox() const override
	{
		return bbox;
	}

	bool Hit(const Ray& r, Interval t, HitRecord& rec) const override
	{
		auto oc = center - r.Origin();
		auto a = r.Direction().SqrLength();
		auto h = Dot(r.Direction(), oc);
		auto c = oc.SqrLength() - radius * radius;
		auto discriminant = h * h - a * c;
		if (discriminant < 0)
			return false;

		auto sqrtDiscriminant = std::sqrt(discriminant);
		auto root = (h - sqrtDiscriminant) / a;
		if (!t.Surrounds(root))
		{
			root = (h + sqrtDiscriminant) / a;
			if (!t.Surrounds(root))
				return false;
		}

		rec.t = root;
		rec.p = r.PointAtT(rec.t);
		Vector3 outwardNormal = (rec.p - center) / radius;
		rec.SetFaceNormal(r, outwardNormal);
		GetSphereUV(outwardNormal, rec.u, rec.v);
		rec.mat = mat;

		return true;
	}

private:
	Vector3 center;
	double radius;
	std::shared_ptr<Material> mat;

	AABB bbox;

	static void GetSphereUV(const Vector3& p, double& u, double& v)
	{
		// p: a given point on the sphere of radius one, centered at the origin.
		// u: returned value [0,1] of angle around the Y axis from X=-1.
		// v: returned value [0,1] of angle from Y=-1 to Y=+1.
		//     <1 0 0> yields <0.50 0.50>       <-1  0  0> yields <0.00 0.50>
		//     <0 1 0> yields <0.50 1.00>       < 0 -1  0> yields <0.50 0.00>
		//     <0 0 1> yields <0.25 0.50>       < 0  0 -1> yields <0.75 0.50>

		auto theta = std::acos(-p.y());
		auto phi = std::atan2(-p.z(), p.x()) + pi;
		u = phi / (2 * pi);
		v = theta / pi;
	}

};

class Quad : public Hittable
{
public:

	Quad(const Vector3& Q, const Vector3& u, const Vector3& v, std::shared_ptr<Material> mat)
		: Q(Q), u(u), v(v), mat(mat)
	{
		auto n = Cross(u, v);
		normal = UnitVector(n);
		D = Dot(normal, Q);
		w = n / Dot(n, n);

		SetBoundingBox();
	}

	virtual void SetBoundingBox()
	{
		auto bboxDiagonal1 = AABB(Q, Q + u + v);
		auto bboxDiagonal2 = AABB(Q + u, Q + v);
		bbox = AABB(bboxDiagonal1, bboxDiagonal2);
	}

	AABB BoundingBox() const override
	{
		return bbox;
	}

	bool Hit(const Ray& r, Interval t, HitRecord& rec) const override
	{
		auto denom = Dot(normal, r.Direction());
		if(std::fabs(denom) < 1e-8)
			return false;

		auto hitT = (D - Dot(normal, r.Origin())) / denom;
		if (!t.Contains(hitT))
			return false;

		auto intersection = r.PointAtT(hitT);
		Vector3 planarHitPoint = intersection - Q;
		auto alpha = Dot(w, Cross(planarHitPoint, v));
		auto beta = Dot(w, Cross(u, planarHitPoint));

		if (!IsInterior(alpha, beta, rec))
			return false;

		rec.t = hitT;
		rec.p = intersection;
		rec.mat = mat;
		rec.SetFaceNormal(r, normal);

		return true;
	}

private:
	Vector3 Q;
	Vector3 u, v;
	Vector3 w;
	std::shared_ptr<Material> mat;
	AABB bbox;
	Vector3 normal;
	double D;

	virtual bool IsInterior(double alpha, double beta, HitRecord& rec) const
	{
		Interval unitInterval = Interval(0, 1);
		// Given the hit point in plane coordinates, return false if it is outside the
		// primitive, otherwise set the hit record UV coordinates and return true.

		if (!unitInterval.Contains(alpha) || !unitInterval.Contains(beta))
			return false;

		rec.u = alpha;
		rec.v = beta;
		return true;
	}
};

class Triangle : public Hittable
{
public:

	Triangle(const Vertex& vertA, const Vertex& vertB, const Vertex& vertC, std::shared_ptr<Material> mat)
		: mat(mat)
	{
		Vector3 aPos = Vector3(vertA.m_position[0], vertA.m_position[1], vertA.m_position[2]);
		Vector3 bPos = Vector3(vertB.m_position[0], vertB.m_position[1], vertB.m_position[2]);
		Vector3 cPos = Vector3(vertC.m_position[0], vertC.m_position[1], vertC.m_position[2]);

		A = aPos;
		B = bPos;
		C = cPos;

		uvA[0] = vertA.m_uvX;
		uvA[1] = vertA.m_uvY;

		uvB[0] = vertB.m_uvX;
		uvB[1] = vertB.m_uvY;

		uvC[0] = vertC.m_uvX;
		uvC[1] = vertC.m_uvY;

		auto ab = B - A;
		auto ac = C - A;
		auto n = Cross(ab, ac);
		normal = -UnitVector(n);

		Vector3 minPt(
			std::min({ aPos.x(), bPos.x(), cPos.x() }),
			std::min({ aPos.y(), bPos.y(), cPos.y() }),
			std::min({ aPos.z(), bPos.z(), cPos.z() })
		);
		Vector3 maxPt(
			std::max({ aPos.x(), bPos.x(), cPos.x() }),
			std::max({ aPos.y(), bPos.y(), cPos.y() }),
			std::max({ aPos.z(), bPos.z(), cPos.z() })
		);
		bbox = AABB(minPt, maxPt);
	}

	bool Hit(const Ray& r, Interval t, HitRecord& rec) const override
	{
		Vector3 edge1 = B - A;
		Vector3 edge2 = C - A;
		Vector3 rayCross = Cross(r.Direction(), edge2);
		float det = Dot(edge1, rayCross);
		if (std::fabs(det) < 1e-8)
			return false;

		float invDet = 1.0f / det;
		Vector3 s = r.Origin() - A;
		float u = invDet * Dot(s, rayCross);

		if((u < 0 && abs(u) > 1e-8) || (u > 1 && abs(u - 1) > 1e-8))
			return false;

		Vector3 sCross = Cross(s, edge1);
		float v = invDet * Dot(r.Direction(), sCross);
		if ((v < 0 && abs(v) > 1e-8) || (u + v > 1 && abs(u + v - 1) > 1e-8))
			return false;

		float hitT = invDet * Dot(edge2, sCross);
		if (hitT > 1e-8)
		{
			rec.t = hitT;
			rec.p = r.PointAtT(hitT);
			rec.mat = mat;
			rec.SetFaceNormal(r, normal);

			GetUVCoordinates(rec.p, rec.u, rec.v);
		}
		else
			return false;
	}

	AABB BoundingBox() const override
	{
		return bbox;
	}

private:

	Vector3 A, B, C;
	Vector3 normal;
	float uvA[2];
	float uvB[2];
	float uvC[2];

	AABB bbox;
	std::shared_ptr<Material> mat;

	void GetUVCoordinates(const Vector3& p, double& u, double& v) const
	{
		// Barycentric coordinates
		Vector3 v0 = B - A;
		Vector3 v1 = C - A;
		Vector3 v2 = p - A;
		auto d00 = Dot(v0, v0);
		auto d01 = Dot(v0, v1);
		auto d11 = Dot(v1, v1);
		auto d20 = Dot(v2, v0);
		auto d21 = Dot(v2, v1);
		auto denom = d00 * d11 - d01 * d01;
		auto vCoord = (d11 * d20 - d01 * d21) / denom;
		auto wCoord = (d00 * d21 - d01 * d20) / denom;
		u = vCoord;
		v = wCoord;
	}
};

class HittableList : public Hittable
{
public:
	std::vector<std::shared_ptr<Hittable>> objects;

	HittableList() {};
	HittableList(std::shared_ptr<Hittable> object)
	{
		Add(object);
	}

	void Add(std::shared_ptr<Hittable> object)
	{
		objects.push_back(object);
		bbox = AABB(bbox, object->BoundingBox());
	}

	bool Hit(const Ray& r, Interval t, HitRecord& rec) const override
	{
		HitRecord tempRec;
		bool hitAnything = false;
		double closestSoFar = t.max;
		for (const auto& object : objects)
		{
			if (object->Hit(r, Interval(t.min, closestSoFar), tempRec))
			{
				hitAnything = true;
				closestSoFar = tempRec.t;
				rec = tempRec;
			}
		}
		return hitAnything;
	}

	AABB BoundingBox() const override
	{
		return bbox;
	}

private:

	AABB bbox;
};

class TriangleMesh : public HittableList
{
public:

	TriangleMesh(const std::vector<Vertex>& vertices, std::shared_ptr<Material> mat)
	{
		if (vertices.size() % 3 != 0)
			throw std::exception("TriangleMesh: vertex count not multiple of 3");
		for (size_t i = 0; i < vertices.size(); i += 3)
		{
			Add(std::make_shared<Triangle>(vertices[i], vertices[i + 1], vertices[i + 2], mat));
		}
	}
};

#endif