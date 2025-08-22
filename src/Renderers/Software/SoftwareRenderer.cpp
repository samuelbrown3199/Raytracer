#include "SoftwareRenderer.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

void WriteImage(const std::string& filename, const std::vector<uint8_t>& pixels, int width, int height)
{
	stbi_write_png(filename.c_str(), width, height, 4, pixels.data(), width * 4);
}

void SoftwareRenderer::InitializeWorld()
{
	auto materialGround = std::make_shared<Lambertian>(Vector3(0.5, 0.5, 0.5));
	m_world.Add(std::make_shared<Sphere>(Vector3(0, -1000, 0), 1000, materialGround));

	for(int i = -11; i < 11; ++i)
	{
		for (int j = -11; j < 11; ++j)
		{
			auto chooseMat = RandomDouble();
			Vector3 center(i + 0.9 * RandomDouble(), 0.2, j + 0.9 * RandomDouble());

			if ((center - Vector3(4, 0.2, 0)).Length() > 0.9)
			{
				std::shared_ptr<Material> sphereMaterial;

				if(chooseMat < 0.8) // Diffuse
				{
					auto albedo = Vector3::Random() * Vector3::Random();
					sphereMaterial = std::make_shared<Lambertian>(albedo);
					m_world.Add(std::make_shared<Sphere>(center, 0.2, sphereMaterial));
				}
				else if (chooseMat < 0.95) // Metal
				{
					auto albedo = Vector3::Random(0.5, 1);
					auto fuzz = RandomDouble(0, 0.5);
					sphereMaterial = std::make_shared<Metal>(albedo, fuzz);
					m_world.Add(std::make_shared<Sphere>(center, 0.2, sphereMaterial));
				}
				else // Glass
				{
					sphereMaterial = std::make_shared<Dielectric>(1.5);
					m_world.Add(std::make_shared<Sphere>(center, 0.2, sphereMaterial));
				}
			}
		}
	}

	auto material1 = std::make_shared<Dielectric>(1.5);
	m_world.Add(std::make_shared<Sphere>(Vector3(0, 1, 0), 1.0, material1));

	auto material2 = std::make_shared<Lambertian>(Vector3(0.4, 0.2, 0.1));
	m_world.Add(std::make_shared<Sphere>(Vector3(-4, 1, 0), 1.0, material2));

	auto material3 = std::make_shared<Metal>(Vector3(0.7, 0.6, 0.5), 0.0);
	m_world.Add(std::make_shared<Sphere>(Vector3(4, 1, 0), 1.0, material3));

	m_world = HittableList(std::make_shared<BVHNode>(m_world));
}

void SoftwareRenderer::RenderImage()
{
	auto startTime = std::chrono::high_resolution_clock::now();

	auto aspectRatio = 16.0f / 9.0f;
	
	Camera camera;
	camera.aspectRatio = aspectRatio;
	camera.m_iWidth = 400;
	camera.m_iSamplesPerPixel = 100;
	camera.m_iMaxDepth = 50;

	camera.m_fov = 20.0f;
	camera.m_lookFrom = Vector3(13, 2, 3);
	camera.m_lookAt = Vector3(0, 0, 0);
	camera.m_up = Vector3(0, 1, 0);

	camera.m_defocusAngle = 0.6f;
	camera.m_focusDistance = 10.0f;
	camera.Initialize();

	camera.Render(m_world);

	auto endTime = std::chrono::high_resolution_clock::now();
	double duration = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count() / 1000.0f);

	std::clog << "Rendering took " << duration << " seconds.\n";
}