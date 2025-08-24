#pragma once

#include <iostream>
#include <vector>
#include <array>

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include "../../Useful/Useful.h"

class EngineManager;

/**
*Handles input for the engine.
*/
class InputManager
{
private:

	std::function<void()> m_handleQuit;

	void RemoveKeyFromOldDownKeys(SDL_Scancode key);

public:
	/**
	*Stores the mouse x and y positions.
	*/
	float m_fMouseX = 0, m_fMouseY = 0;
	float m_mouseDeltaX = 0, m_mouseDeltaY = 0;

	glm::vec2 m_iWorldMousePos;
	std::vector<SDL_Scancode> m_vDownKeys, m_vOldDownKeys;
	std::vector<SDL_Scancode> m_vUpKeys;
	std::vector<int> m_vDownMouseButtons, m_vOldMouseButtons;
	std::vector<int> m_vUpMouseButtons;


	void HandleGeneralInput();
	void SetQuitFunction(std::function<void()> handleQuit){ m_handleQuit = handleQuit; }

	/**
	*Checks if a key is down. Uses SDL keycodes which can be found here: https://wiki.libsdl.org/SDL_Scancode
	*/
	bool GetKey(SDL_Keycode _key);
	/**
	*Checks if the key was pressed this frame.
	*/
	bool GetKeyDown(SDL_Keycode _key);
	/**
	*Checks if the key was released this frame.
	*/
	bool GetKeyUp(SDL_Keycode _key);
	/**
	*Checks if a mouse button is down. Button 1 is left, 3 is right, 2 is middle.
	*/
	bool GetMouseButton(int _button);
	/**
	*Checks if the mouse button has been pressed this frame.
	*/
	bool GetMouseButtonDown(int _button);
	/**
	*Checks if the mouse button has been released this frame.
	*/
	bool GetMouseButtonUp(int _button);
	/**
	*Gets the mouse position and stores it in the mouse position variables.
	*/
	void GetMousePosition();
	/**
	*Clears the keys that are currently up and down.
	*/
	void ClearFrameInputs();
};