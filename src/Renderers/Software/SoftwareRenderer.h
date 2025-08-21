#pragma once

#include "Common.h"
#include "Material.h"

#include <vector>
#include <memory>

void WriteImage(const std::string& filename, const std::vector<uint8_t>& pixels, int width, int height);

class Camera
{
public:

	double aspectRatio = 1.0;
	int m_iWidth;
	int m_iSamplesPerPixel = 10; // Number of samples per pixel
	int m_iMaxDepth = 10; //Maximum number of ray bounces

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

				Colour3 pixelColour = Vector3(0,0,0);
				for(int sample = 0; sample < m_iSamplesPerPixel; ++sample)
				{
					Ray r = GetRay(x, y);
					pixelColour += RayColour(r, m_iMaxDepth, world);
				}
				pixelColour *= m_pixelSampleScale;

				auto r = LinearToGamma(pixelColour[0]);
				auto g = LinearToGamma(pixelColour[1]);
				auto b = LinearToGamma(pixelColour[2]);
				auto a = 255.999 * 1.0;

				static const Interval intensity(0.000, 0.999);
	
				int i = (y * m_iWidth + x) * 4;
				pixels[i + 0] = int(256 * intensity.Clamp(r));
				pixels[i + 1] = int(256 * intensity.Clamp(g));
				pixels[i + 2] = int(256 * intensity.Clamp(b));
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

		m_pixelSampleScale = 1.0 / m_iSamplesPerPixel;

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
	double m_pixelSampleScale;

	Colour3 RayColour(const Ray& r, int depth, const HittableList& world)
	{
		if (depth <= 0)
			return Vector3(0, 0, 0);

		HitRecord rec;
		if (world.Hit(r, Interval(0.001, infinity), rec))
		{
			Ray scattered;
			Vector3 attenuation;
			if(rec.mat->Scatter(r, rec, attenuation, scattered))
				return attenuation * RayColour(scattered, depth - 1, world);

			return Vector3(0, 0, 0); // No scattering, return black
		}

		// Simple gradient background
		auto unitDirection = UnitVector(r.Direction());
		auto a = 0.5 * (unitDirection.y() + 1.0);
		return (1.0 - a) * Colour3(1.0, 1.0, 1.0) + a * Colour3(0.5, 0.7, 1.0);
	}

	Ray GetRay(int x, int y) const
	{
		auto offset = SampleSquare();
		auto pixel_sample = m_pixel00Location
			+ ((x + offset.x()) * m_pixelDeltaU)
			+ ((y + offset.y()) * m_pixelDeltaV);

		auto ray_origin = m_center;
		auto ray_direction = pixel_sample - ray_origin;

		return Ray(ray_origin, ray_direction);
	}

	Vector3 SampleSquare() const
	{
		// Returns the vector to a random point in the [-.5,-.5]-[+.5,+.5] unit square.
		return Vector3(RandomDouble() - 0.5, RandomDouble() - 0.5, 0);
	}
};

class SoftwareRenderer
{


	HittableList m_world;

public:

	void InitializeWorld();
	void RenderImage();
};