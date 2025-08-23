#include <iostream>

#include "Renderers/Software/SoftwareRenderer.h"
#include "Useful/Useful.h"

int MainMenu()
{
	int mode = 0;

	std::cout << "Select raytracer mode:\n\n";
	std::cout << "0: Software renderer (output one image file)\n";
	std::cout << "\n";

	std::cout << "----------------------------------------------\n\n";

	std::cout << "Enter mode number: ";

	std::cin >> mode;
	return mode;
}

int SceneSelectionMenu()
{
	int scene = 0;
	std::cout << "Select scene:\n\n";
	std::cout << "0: Sphere scene\n";
	std::cout << "1: Checkered Spheres\n";
	std::cout << "2: Earth\n";
	std::cout << "3: Quads\n";
	std::cout << "4: Simple Light\n";
	std::cout << "5: Cornell Box\n";
	std::cout << "6: Gold Dragon and Spheres\n";
	std::cout << "7: Cottage Scene\n";
	std::cout << "\n";
	std::cout << "----------------------------------------------\n\n";
	std::cout << "Enter scene number: ";
	std::cin >> scene;
	return scene;
}

int main()
{
	int mode = MainMenu();
	std::cout << "\n\n";

	switch (mode)
	{
	case 0:
		{
			SoftwareRenderer renderer;

			int scene = SceneSelectionMenu();
			std::cout << "\n\n";

			CreateNewDirectory("SoftwareRenders");

			renderer.InitializeWorld(scene);
			renderer.RenderImage();

			std::cout << "\nPress ENTER to exit.";
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			std::cin.get();
		}
		break;
	default:
		std::string errorMessage = "Unknown raytracer mode: " + mode;
		std::cout << errorMessage << std::endl;
		break;
	}

	return 0;
}