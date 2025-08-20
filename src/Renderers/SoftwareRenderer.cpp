#include "SoftwareRenderer.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <iostream>
#include <vector>
#include <chrono>

#include "../Maths/Intersections.hpp"
#include "../Maths/Ray.hpp" 

Colour3 RayColour(const Ray& r)
{
	auto t = HitSphere(Vector3(0, 0, -1), 0.5, r);
	if(t > 0.0)
	{
		Vector3 normal = UnitVector(r.PointAtT(t) - Vector3(0, 0, -1));
		return 0.5 * Colour3(normal.x() + 1, normal.y() + 1, normal.z() + 1);
	}

	// Simple gradient background
	auto unitDirection = UnitVector(r.Direction());
	auto a = 0.5 * (unitDirection.y() + 1.0);
	return (1.0 - a) * Colour3(1.0, 1.0, 1.0) + a * Colour3(0.5, 0.7, 1.0);
}

void SoftwareRenderer::RenderImage()
{
	auto startTime = std::chrono::high_resolution_clock::now();

	auto aspectRatio = 16.0f / 9.0f;
	m_iWidth = 400;
	m_iHeight = static_cast<int>(m_iWidth / aspectRatio);
	m_iHeight = (m_iHeight < 1) ? 1 : m_iHeight;

	std::clog << "Rendering image of size " << m_iWidth << "x" << m_iHeight << std::endl;

	float focalLength = 1.0f;
	float viewportHeight = 2.0f;
	float viewportWidth = viewportHeight * (double(m_iWidth) / m_iHeight);
	Vector3 cameraOrigin(0.0, 0.0, 0.0);

	auto viewportU = Vector3(viewportWidth, 0.0, 0.0);
	auto viewportV = Vector3(0.0, -viewportHeight, 0.0);

	auto pixelDeltaU = viewportU / m_iWidth;
	auto pixelDeltaV = viewportV / m_iHeight;

	auto viewportUpperLeft = cameraOrigin - Vector3(0, 0, focalLength) - viewportU / 2 - viewportV / 2;
	auto pixel00Location = viewportUpperLeft + 0.5 * (pixelDeltaU + pixelDeltaV);

	float percentComplete = 0.0f;
	std::vector<uint8_t> pixels(m_iWidth * m_iHeight * 4);
	for (int y = 0; y < m_iHeight; ++y)
	{
		for (int x = 0; x < m_iWidth; x++)
		{

			auto pixelLocation = pixel00Location + (x * pixelDeltaU) + (y * pixelDeltaV);
			auto rayDirection = pixelLocation - cameraOrigin;

			Ray ray(cameraOrigin, rayDirection);

			Colour3 pixelColour = RayColour(ray);

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

	stbi_write_png("output.png", m_iWidth, m_iHeight, 4, pixels.data(), m_iWidth * 4);
	std::clog << "\nImage rendering complete. Output saved to 'output.png'.\n";

	auto endTime = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

	std::clog << "Rendering took " << duration.count() << " milliseconds.\n";
}