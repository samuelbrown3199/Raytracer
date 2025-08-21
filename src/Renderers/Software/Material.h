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

class Dielectric : public Material
{
public:

	Dielectric(double refractionIndex) : refractionIndex(refractionIndex) {}

	bool Scatter(const Ray& rIn, const HitRecord& rec, Vector3& attenuation, Ray& scattered) const override
	{
		attenuation = Vector3(1.0, 1.0, 1.0);
		double ri = rec.frontFace ? (1.0 / refractionIndex) : refractionIndex;

		Vector3 unitDirection = UnitVector(rIn.Direction());
		double cosTheta = std::fmin(Dot(-unitDirection, rec.normal), 1.0);
		double sinTheta = std::sqrt(1.0 - cosTheta * cosTheta);

		bool cannotRefract = ri * sinTheta > 1.0;
		Vector3 direction;
		if (cannotRefract||Reflectance(cosTheta, refractionIndex) > RandomDouble())
			direction = Reflect(unitDirection, rec.normal);
		else
			direction = Refract(unitDirection, rec.normal, ri);

		scattered = Ray(rec.p, direction);
		return true;
	}

private:
	double refractionIndex;

	static double Reflectance(double cosine, double refractionIndex)
	{
		// Use Schlick's approximation for reflectance
		double r0 = (1 - refractionIndex) / (1 + refractionIndex);
		r0 *= r0;
		return r0 + (1 - r0) * std::pow((1 - cosine), 5);
	}
};

#endif