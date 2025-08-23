#include "Window.h"

#include "HardwareRenderer.h"

Window::Window(const std::string& windowName, const unsigned int& width, const unsigned int& height)
{
	m_iWidth = width;
	m_iHeight = height;

	m_pWindow = SDL_CreateWindow(windowName.c_str(), m_iWidth, m_iHeight, SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);
	if (!m_pWindow)
		throw std::exception("Failed to create window.");
}

Window::~Window()
{
	SDL_DestroyWindow(m_pWindow);
}

void Window::CheckScreenSizeForUpdate(HardwareRenderer* renderer)
{
	int newWidth, newHeight;
	SDL_GetWindowSize(m_pWindow, &newWidth, &newHeight);
	UpdateScreenSize(renderer, newHeight, newWidth);
}

void Window::UpdateScreenSize(HardwareRenderer* renderer, const int& _height, const int& _width)
{
	if (_height < 0 || _width < 0)
		return;

	int newWidth = _width, newHeight = _height;
	if (newHeight == 0 || newWidth == 0)
	{
		const SDL_DisplayMode* displayMode = SDL_GetDesktopDisplayMode(0);

		newHeight = displayMode->h;
		newWidth = displayMode->w;
	}

	if (newWidth != m_iWidth || newHeight != m_iHeight)
	{
		m_iWidth = newWidth;
		m_iHeight = newHeight;

		SDL_SetWindowSize(m_pWindow, m_iWidth, m_iHeight);

		renderer->RecreateSwapchain();
	}
}

void Window::SetWindowFullScreen(HardwareRenderer* renderer, const int& _mode)
{
	switch (_mode)
	{
	case 0:
		SDL_SetWindowBordered(m_pWindow, true);
		SDL_SetWindowFullscreen(m_pWindow, 0);
		break;
	case 2:
		SDL_SetWindowBordered(m_pWindow, false);
	case 1:
		SDL_SetWindowFullscreen(m_pWindow, SDL_WINDOW_FULLSCREEN);
		break;
	case 3:
		SDL_SetWindowBordered(m_pWindow, true);
		SDL_MaximizeWindow(m_pWindow);
		break;
	}

	CheckScreenSizeForUpdate(renderer);
}