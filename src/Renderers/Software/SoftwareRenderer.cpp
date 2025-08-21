#include "SoftwareRenderer.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

void WriteImage(const std::string& filename, const std::vector<uint8_t>& pixels, int width, int height)
{
	stbi_write_png(filename.c_str(), width, height, 4, pixels.data(), width * 4);
}

void SoftwareRenderer::InitializeWorld()
{
	auto materialGround = std::make_shared<Lambertian>(Vector3(0.8, 0.8, 0.0));
	auto materialCenter = std::make_shared<Lambertian>(Vector3(0.1, 0.2, 0.5));
	auto materialLeft = std::make_shared<Dielectric>(1.50);
	auto materialBubble = std::make_shared<Dielectric>(1.00 / 1.50);
	auto materialRight = std::make_shared<Metal>(Vector3(0.8, 0.6, 0.2), 1.0);

	m_world.Add(std::make_shared<Sphere>(Vector3(0, -100.5, -1), 100, materialGround));
	m_world.Add(std::make_shared<Sphere>(Vector3(0, 0, -1.2), 0.5, materialCenter));
	m_world.Add(std::make_shared<Sphere>(Vector3(-1, 0, -1.0), 0.5, materialLeft));
	m_world.Add(std::make_shared<Sphere>(Vector3(-1, 0, -1.0), 0.40, materialBubble));
	m_world.Add(std::make_shared<Sphere>(Vector3(1, 0, -1.0), 0.5, materialRight));
}

void SoftwareRenderer::RenderImage()
{
	auto startTime = std::chrono::high_resolution_clock::now();

	auto aspectRatio = 16.0f / 9.0f;
	
	Camera camera;
	camera.aspectRatio = aspectRatio;
	camera.m_iWidth = 720;
	camera.m_iSamplesPerPixel = 100;
	camera.m_iMaxDepth = 50;
	camera.Initialize();

	camera.Render(m_world);

	auto endTime = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

	std::clog << "Rendering took " << duration.count() << " milliseconds.\n";
}