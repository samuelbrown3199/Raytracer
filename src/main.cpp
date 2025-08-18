#include <iostream>

#include <SDL3/SDL.h>

int main()
{
	std::cout << "Hello, World!" << std::endl;

	SDL_Window* window = SDL_CreateWindow("Hello World", 800, 600, SDL_WINDOW_VULKAN);
	if (!window) {
		std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
		return 1;
	}

	bool run = true;
	SDL_Event event;
	while (run)
	{
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_EVENT_QUIT)
			{
				run = false;
				break;
			}
		}
	}

	return 0;
}