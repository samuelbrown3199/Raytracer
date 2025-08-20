#pragma once

#include "Common.h"

#include <vector>
#include <memory>

void WriteImage(const std::string& filename, const std::vector<uint8_t>& pixels, int width, int height);

class HitRecord
{
public:

	Vector3 p;
	Vector3 normal;
	double t;
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
};

class Sphere : public Hittable
{
public:

	Sphere(const Vector3& center, double radius)
		: center(center), radius(radius) {
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
		return true;
	}

private:
	Vector3 center;
	double radius;
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
};

class Camera
{
public:

	double aspectRatio = 1.0;
	int m_iWidth;

	void Render(const HittableList& world)
	{
		float percentComplete = 0.0f;
		std::vector<uint8_t> pixels(m_iWidth * m_iHeight * 4);
		for (int y = 0; y < m_iHeight; ++y)
		{
			for (int x = 0; x < m_iWidth; x++)
			{

				auto pixelLocation = m_pixel00Location + (x * m_pixelDeltaU) + (y * m_pixelDeltaV);
				auto rayDirection = pixelLocation - m_center;

				Ray ray(m_center, rayDirection);

				Colour3 pixelColour = RayColour(ray, world);

				auto a = 255.999 * 1.0;

				Colour3 Colour255 = Get255Color(pixelColour);

				int i = (y * m_iWidth + x) * 4;
				pixels[i + 0] = Colour255.x();
				pixels[i + 1] = Colour255.y();
				pixels[i + 2] = Colour255.z();
				pixels[i + 3] = static_cast<uint8_t>(a);
			}

			// Calculate the percentage of completion
			percentComplete = static_cast<float>(y + 1) / m_iHeight * 100.0f;
			std::clog << "\rRendering progress: " << percentComplete << "%       " << std::flush;
		}

		WriteImage("output.png", pixels, m_iWidth, m_iHeight);
		std::clog << "\nImage rendering complete. Output saved to 'output.png'.\n";
	}

	void Initialize()
	{
		m_iHeight = static_cast<int>(m_iWidth / aspectRatio);
		m_iHeight = (m_iHeight < 1) ? 1 : m_iHeight;

		float focalLength = 1.0f;
		float viewportHeight = 2.0f;
		float viewportWidth = viewportHeight * (double(m_iWidth) / m_iHeight);
		m_center = Vector3(0.0, 0.0, 0.0);

		auto viewportU = Vector3(viewportWidth, 0.0, 0.0);
		auto viewportV = Vector3(0.0, -viewportHeight, 0.0);

		m_pixelDeltaU = viewportU / m_iWidth;
		m_pixelDeltaV = viewportV / m_iHeight;

		auto viewportUpperLeft = m_center - Vector3(0, 0, focalLength) - viewportU / 2 - viewportV / 2;
		m_pixel00Location = viewportUpperLeft + 0.5 * (m_pixelDeltaU + m_pixelDeltaV);
	}

private:

	int m_iHeight;
	Vector3 m_center;         // Camera center
	Vector3 m_pixel00Location;    // Location of pixel 0, 0
	Vector3   m_pixelDeltaU;  // Offset to pixel to the right
	Vector3   m_pixelDeltaV;  // Offset to pixel below

	Colour3 RayColour(const Ray& r, const HittableList& world)
	{
		HitRecord rec;
		if (world.Hit(r, Interval(0, infinity), rec))
		{
			return 0.5 * Colour3(rec.normal.x() + 1, rec.normal.y() + 1, rec.normal.z() + 1);
		}

		// Simple gradient background
		auto unitDirection = UnitVector(r.Direction());
		auto a = 0.5 * (unitDirection.y() + 1.0);
		return (1.0 - a) * Colour3(1.0, 1.0, 1.0) + a * Colour3(0.5, 0.7, 1.0);
	}
};

class SoftwareRenderer
{


	HittableList m_world;

public:

	void InitializeWorld();
	void RenderImage();
};