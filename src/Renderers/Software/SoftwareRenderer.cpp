#include "SoftwareRenderer.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

void WriteImage(const std::string& filename, const std::vector<uint8_t>& pixels, int width, int height)
{
	if(stbi_write_png(filename.c_str(), width, height, 3, pixels.data(), width * 3) != 0)
		std::clog << "\nImage written to " << filename << "\n";
	else
		std::clog << "\nFailed to write image to " << filename << "\n";
}

void SoftwareRenderer::SphereScene()
{
	auto checker = std::make_shared<CheckerTexture>(0.32, Vector3(0.2, 0.3, 0.1), Vector3(0.9, 0.9, 0.9));
	auto materialGround = std::make_shared<Lambertian>(checker);
	m_world.Add(std::make_shared<Sphere>(Vector3(0, -1000, 0), 1000, materialGround));

	for (int i = -11; i < 11; ++i)
	{
		for (int j = -11; j < 11; ++j)
		{
			auto chooseMat = RandomDouble();
			Vector3 center(i + 0.9 * RandomDouble(), 0.2, j + 0.9 * RandomDouble());

			if ((center - Vector3(4, 0.2, 0)).Length() > 0.9)
			{
				std::shared_ptr<Material> sphereMaterial;

				if (chooseMat < 0.8) // Diffuse
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

	m_camera.m_background = Vector3(0.70, 0.80, 1.00);
	m_camera.m_fov = 20.0f;
	m_camera.m_lookFrom = Vector3(13, 2, 3);
	m_camera.m_lookAt = Vector3(0, 0, 0);
	m_camera.m_up = Vector3(0, 1, 0);

	m_camera.m_defocusAngle = 0.6f;
	m_camera.m_focusDistance = 10.0f;
}

void SoftwareRenderer::CheckeredSpheres()
{
	auto checker = std::make_shared<CheckerTexture>(0.32, Vector3(.2, .3, .1), Vector3(.9, .9, .9));

	m_world.Add(std::make_shared<Sphere>(Vector3(0, -10, 0), 10, make_shared<Lambertian>(checker)));
	m_world.Add(std::make_shared<Sphere>(Vector3(0, 10, 0), 10, make_shared<Lambertian>(checker)));

	m_camera.m_background = Vector3(0.70, 0.80, 1.00);
	m_camera.m_fov = 20.0f;
	m_camera.m_lookFrom = Vector3(13, 2, 3);
	m_camera.m_lookAt = Vector3(0, 0, 0);
	m_camera.m_up = Vector3(0, 1, 0);

	m_camera.m_defocusAngle = 0.6f;
	m_camera.m_focusDistance = 10.0f;
}

void SoftwareRenderer::EarthScene()
{
	auto earthTexture = std::make_shared<ImageTexture>("Resources/Textures/earthmap.jpg");
	auto earthSurface = std::make_shared<Lambertian>(earthTexture);

	auto globe = std::make_shared<Sphere>(Vector3(0, 0, 0), 2, earthSurface);

	m_world.Add(globe);

	m_camera.m_background = Vector3(0.70, 0.80, 1.00);
	m_camera.m_fov = 20.0f;
	m_camera.m_lookFrom = Vector3(0, 0, 12);
	m_camera.m_lookAt = Vector3(0, 0, 0);
	m_camera.m_up = Vector3(0, 1, 0);
	m_camera.m_defocusAngle = 0.0f;
}

void SoftwareRenderer::QuadsScene()
{
	auto leftRed = std::make_shared<Lambertian>(Vector3(1.0, 0.2, 0.2));
	auto backGreen = std::make_shared<Lambertian>(Vector3(0.2, 1.0, 0.2));
	auto rightBlue = std::make_shared<Lambertian>(Vector3(0.2, 0.2, 1.0));
	auto upperOrange = std::make_shared<Lambertian>(Vector3(1.0, 0.5, 0.0));
	auto lowerTeal = std::make_shared<Lambertian>(Vector3(0.2, 0.8, 0.8));

	m_world.Add(std::make_shared<Quad>(Vector3(-3, -2, 5), Vector3(0, 0, -4), Vector3(0, 4, 0), leftRed));
	m_world.Add(std::make_shared<Quad>(Vector3(-2, -2, 0), Vector3(4, 0, 0), Vector3(0, 4, 0), backGreen));
	m_world.Add(std::make_shared<Quad>(Vector3(3, -2, 1), Vector3(0, 0, 4), Vector3(0, 4, 0), rightBlue));
	m_world.Add(std::make_shared<Quad>(Vector3(-2, 3, 1), Vector3(4, 0, 0), Vector3(0, 0, 4), upperOrange));
	m_world.Add(std::make_shared<Quad>(Vector3(-2, -3, 5), Vector3(4, 0, 0), Vector3(0, 0, -4), lowerTeal));

	m_camera.m_background = Vector3(0.70, 0.80, 1.00);
	m_camera.m_fov = 80.0f;
	m_camera.m_lookFrom = Vector3(0, 0, 9);
	m_camera.m_lookAt = Vector3(0, 0, 0);
	m_camera.m_up = Vector3(0, 1, 0);
	m_camera.m_defocusAngle = 0.0f;
}

void SoftwareRenderer::SimpleLightScene()
{
	auto sphereMat = std::make_shared<Lambertian>(Vector3(0.1, 0.2, 0.5));
	auto groundMat = std::make_shared<Lambertian>(Vector3(0.4, 0.2, 0.1));

	m_world.Add(std::make_shared<Sphere>(Vector3(0, -1000, -1), 1000, groundMat));
	m_world.Add(std::make_shared<Sphere>(Vector3(0, 2, 0), 2, sphereMat));

	auto difflight = std::make_shared<DiffuseLight>(Vector3(4, 4, 4));
	m_world.Add(std::make_shared<Sphere>(Vector3(0, 7, 0), 2, difflight));
	m_world.Add(std::make_shared<Quad>(Vector3(3, 1, -2), Vector3(2, 0, 0), Vector3(0, 2, 0), difflight));

	m_camera.m_background = Vector3(0.0, 0.0, 0.0);
	m_camera.m_fov = 20.0f;
	m_camera.m_lookFrom = Vector3(26, 3, 6);
	m_camera.m_lookAt = Vector3(0, 2, 0);
	m_camera.m_up = Vector3(0, 1, 0);
	m_camera.m_defocusAngle = 0.0f;
}

void SoftwareRenderer::CornellBoxScene()
{
	auto red = std::make_shared<Lambertian>(Vector3(.65, .05, .05));
	auto white = std::make_shared<Lambertian>(Vector3(.73, .73, .73));
	auto green = std::make_shared<Lambertian>(Vector3(.12, .45, .15));
	auto light = std::make_shared<DiffuseLight>(Vector3(15, 15, 15));

	m_world.Add(std::make_shared<Quad>(Vector3(555, 0, 0), Vector3(0, 555, 0), Vector3(0, 0, 555), green));
	m_world.Add(std::make_shared<Quad>(Vector3(0, 0, 0), Vector3(0, 555, 0), Vector3(0, 0, 555), red));
	m_world.Add(std::make_shared<Quad>(Vector3(343, 554, 332), Vector3(-130, 0, 0), Vector3(0, 0, -105), light));
	m_world.Add(std::make_shared<Quad>(Vector3(0, 0, 0), Vector3(555, 0, 0), Vector3(0, 0, 555), white));
	m_world.Add(std::make_shared<Quad>(Vector3(555, 555, 555), Vector3(-555, 0, 0), Vector3(0, 0, -555), white));
	m_world.Add(std::make_shared<Quad>(Vector3(0, 0, 555), Vector3(555, 0, 0), Vector3(0, 555, 0), white));

	m_world.Add(std::make_shared<Sphere>(Vector3(277.5, 277.5, 277.5), 150, white));

	m_camera.m_background = Vector3(0, 0, 0);
	m_camera.m_fov = 40;
	m_camera.m_lookFrom = Vector3(278, 278, -800);
	m_camera.m_lookAt = Vector3(278, 278, 0);
	m_camera.m_up = Vector3(0, 1, 0);
	m_camera.m_defocusAngle = 0;
}

void SoftwareRenderer::TriangleMeshScene()
{
	auto red = std::make_shared<Lambertian>(Vector3(.65, .05, .05));

	m_world.Add(std::make_shared<Triangle>(Vector3(-0.5, -0.5, 0), Vector3(1.5, -0.5, 0), Vector3(1.0, 1.5, 0), red));

	m_camera.m_background = Vector3(0.70, 0.80, 1.00);
	m_camera.m_fov = 80.0f;
	m_camera.m_lookFrom = Vector3(0, 0, 9);
	m_camera.m_lookAt = Vector3(0, 0, 0);
	m_camera.m_up = Vector3(0, 1, 0);
	m_camera.m_defocusAngle = 0;
}

void SoftwareRenderer::GoldDragonAndSpheresScene()
{
	auto groundMat = std::make_shared<Lambertian>(Vector3(0.4, 0.2, 0.1));
	m_world.Add(std::make_shared<Sphere>(Vector3(0, -1000, -1), 997.5, groundMat));

	for (int i = -11; i < 11; ++i)
	{
		for (int j = -11; j < 11; ++j)
		{
			auto chooseMat = RandomDouble();
			Vector3 center(i + 0.9 * RandomDouble(), -2.5, j + 0.9 * RandomDouble());

			if ((center - Vector3(4, 0.2, 0)).Length() > 0.9)
			{
				std::shared_ptr<Material> sphereMaterial;

				if (chooseMat < 0.8) // Diffuse
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

	std::vector<Vertex> vertices;
	LoadObjFile("Resources/Models/dragon-lowres.obj", vertices);

	auto mat = std::make_shared<Metal>(Vector3(0.94, 0.72, 0.14), 0.2);
	TriangleMesh dragon = TriangleMesh(vertices, mat);
	m_world.Add(std::make_shared<BVHNode>(dragon));

	m_camera.m_background = Vector3(0.70, 0.80, 1.00);
	m_camera.m_fov = 90.0f;
	m_camera.m_lookFrom = Vector3(-1, 2, -5);
	m_camera.m_lookAt = Vector3(0, 0, 0);
	m_camera.m_up = Vector3(0, 1, 0);
	m_camera.m_defocusAngle = 0;
	m_camera.m_focusDistance = 30.0f;
}

void SoftwareRenderer::InitializeWorld(int scene)
{
	switch (scene)
	{
	case 0:
		SphereScene();
		break;
	case 1:
		CheckeredSpheres();
		break;
	case 2:
		EarthScene();
		break;
	case 3:
		QuadsScene();
		break;
	case 4:
		SimpleLightScene();
		break;
	case 5:
		CornellBoxScene();
		break;
	case 6:
		TriangleMeshScene();
		break;
	case 7:
		GoldDragonAndSpheresScene();
		break;
	default:
		std::cout << "Unknown scene " << scene << ", defaulting to sphere scene.\n";
		SphereScene();
		break;
	}
}

void SoftwareRenderer::RenderImage()
{
	auto startTime = std::chrono::high_resolution_clock::now();
	
	auto aspectRatio = 16.0f / 9.0f;
	//auto aspectRatio = 1.0f;

	m_camera.aspectRatio = aspectRatio;
	m_camera.m_iWidth = 1920;
	m_camera.m_iSamplesPerPixel = 500;
	m_camera.m_iMaxDepth = 50;

	m_camera.Initialize();
	m_camera.Render(m_world);

	auto endTime = std::chrono::high_resolution_clock::now();
	double duration = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count() / 1000.0f);
	std::clog << "Rendering took " << duration << " seconds.\n";
}