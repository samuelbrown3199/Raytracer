#ifndef MATERIAL_H
#define MATERIAL_H

#include "Hittable.h"

class Material
{
public:
	virtual ~Material() = default;

	virtual bool Scatter(const Ray& rIn, const HitRecord& rec, Vector3& attenuation, Ray& scattered) const
	{
		return false;
	}
};

class Lambertian : public Material
{
public:
	Lambertian(const Vector3& albedo) : albedo(albedo) {}

	bool Scatter(const Ray& rIn, const HitRecord& rec, Vector3& attenuation, Ray& scattered) const override
	{
		auto scatterDirection = rec.normal + RandomUnitVector();
		if (scatterDirection.NearZero())
			scatterDirection = rec.normal;

		scattered = Ray(rec.p, scatterDirection);
		attenuation = albedo;
		return true;
	}

private:

	Vector3 albedo;
};

class Metal : public Material
{
public:
	Metal(const Vector3& albedo, double fuzz) : albedo(albedo), fuzz(fuzz < 1 ? fuzz : 1) {}

	bool Scatter(const Ray& rIn, const HitRecord& rec, Vector3& attenuation, Ray& scattered) const override
	{
		Vector3 reflected = Reflect(rIn.Direction(), rec.normal);
		reflected = UnitVector(reflected) + (fuzz * RandomUnitVector());
		scattered = Ray(rec.p, reflected);
		attenuation = albedo;
		return (Dot(scattered.Direction(), rec.normal) > 0);
	}

private:
	Vector3 albedo;
	double fuzz;
};

#endif