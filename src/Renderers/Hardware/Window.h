#pragma once

#include <string>

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

class HardwareRenderer;

class Window
{

private:

	unsigned int m_iWidth = 100;
	unsigned int m_iHeight = 100;

	SDL_Window* m_pWindow = nullptr;

public:

	Window(const std::string& windowName, const unsigned int& width, const unsigned int& height);
	~Window();

	/**
	* Returns the window pointer.
	*/
	SDL_Window* GetSDLWindow() const { return m_pWindow; }

	/**
	* Returns the window's SDL window flags.
	*/
	Uint32 GetWindowFlags() const { return SDL_GetWindowFlags(m_pWindow); }

	/**
	* Sets the window's title.
	*/
	void SetWindowTitle(const std::string& title) { SDL_SetWindowTitle(m_pWindow, title.c_str()); }

	/**
	* Returns the window's title.
	*/
	std::string GetWindowTitle() const { return SDL_GetWindowTitle(m_pWindow); }

	/**
	* Returns the window size.
	*/
	glm::ivec2 GetWindowSize() const { return glm::ivec2(m_iWidth, m_iHeight); }

	/**
	* Checks the window size and updates it if it has changed.
	*/
	void CheckScreenSizeForUpdate(HardwareRenderer* renderer);

	/**
	* Updates the window size.
	*/
	void UpdateScreenSize(HardwareRenderer* renderer, const int& _height, const int& _width);

	/**
	* Sets the window to fullscreen or windowed mode. 0 = window, 1 = fullscreen, 2 = borderless windowed.
	*/
	void SetWindowFullScreen(HardwareRenderer* renderer, const int& _mode);

	bool IsWindowMinimized() const { return SDL_GetWindowFlags(m_pWindow) & SDL_WINDOW_MINIMIZED; }

	void SetMouseMode(bool enabled) const
	{
		SDL_SetWindowRelativeMouseMode(m_pWindow, enabled);
	}
};