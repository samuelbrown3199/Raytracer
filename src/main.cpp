#include <iostream>

#include "Renderers/Software/SoftwareRenderer.h"

int main()
{
	int mode = 0;

	std::cout << "Select raytracer mode:\n\n";
	std::cout << "0: Software renderer (output one image file)\n\n";

	std::cout << "----------------------------------------------\n\n";

	std::cout << "Enter mode number: ";

	std::cin >> mode;

	switch (mode)
	{
	case 0:
		{
			SoftwareRenderer renderer;
			renderer.InitializeWorld();
			renderer.RenderImage();
		}
		break;
	default:
		std::string errorMessage = "Unknown raytracer mode: " + mode;
		std::cout << errorMessage << std::endl;
		break;
	}

	return 0;
}