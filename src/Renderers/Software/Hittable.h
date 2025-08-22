#ifndef HITTABLE_H
#define HITTABLE_H

#include "Maths/AABB.h"

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

#endif