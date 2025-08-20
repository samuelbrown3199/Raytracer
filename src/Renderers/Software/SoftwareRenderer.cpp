#include "SoftwareRenderer.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

void WriteImage(const std::string& filename, const std::vector<uint8_t>& pixels, int width, int height)
{
	stbi_write_png(filename.c_str(), width, height, 4, pixels.data(), width * 4);
}

void SoftwareRenderer::InitializeWorld()
{
	m_world.Add(std::make_shared<Sphere>(Vector3(0, 0, -1), 0.5));
	m_world.Add(std::make_shared<Sphere>(Vector3(0, -100.5, -1), 100));
}

void SoftwareRenderer::RenderImage()
{
	auto startTime = std::chrono::high_resolution_clock::now();

	auto aspectRatio = 16.0f / 9.0f;
	
	Camera camera;
	camera.aspectRatio = aspectRatio;
	camera.m_iWidth = 1920;
	camera.Initialize();

	camera.Render(m_world);

	auto endTime = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

	std::clog << "Rendering took " << duration.count() << " milliseconds.\n";
}