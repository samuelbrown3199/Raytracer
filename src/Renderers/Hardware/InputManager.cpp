#include "InputManager.h"

#include <algorithm>
#include <fstream>

#include "Imgui/imgui.h"
#include "Imgui/backends/imgui_impl_sdl3.h"

void InputManager::RemoveKeyFromOldDownKeys(SDL_Scancode key)
{
	for (int i = 0; i < m_vOldDownKeys.size(); i++)
	{
		if (m_vOldDownKeys.at(i) == key)
		{
			m_vOldDownKeys.erase(m_vOldDownKeys.begin() + i);
			return;
		}
	}
}

void InputManager::HandleGeneralInput()
{
	SDL_Event e = { 0 };
	m_mouseDeltaX = 0;
	m_mouseDeltaY = 0;

	while (SDL_PollEvent(&e) != 0)
	{
		ImGui_ImplSDL3_ProcessEvent(&e);
		GetMousePosition();

		if (e.type == SDL_EVENT_QUIT)
		{
			if(m_handleQuit)
				m_handleQuit();
		}

		if (e.type == SDL_EVENT_KEY_DOWN)
		{
			m_vDownKeys.push_back(e.key.scancode);
		}
		else if (e.type == SDL_EVENT_KEY_UP)
		{
			m_vUpKeys.push_back(e.key.scancode);
			RemoveKeyFromOldDownKeys(e.key.scancode);
		}

		if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
		{
			m_vDownMouseButtons.push_back(e.button.button);
		}
		else if (e.type == SDL_EVENT_MOUSE_BUTTON_UP)
		{
			m_vUpMouseButtons.push_back(e.button.button);
		}
		else if (e.type == SDL_EVENT_MOUSE_MOTION)
		{
			m_mouseDeltaX = e.motion.xrel;
			m_mouseDeltaY = e.motion.yrel;
		}
	}
}

bool InputManager::GetKey(SDL_Keycode _key)
{
	const bool* state = SDL_GetKeyboardState(NULL);
	if (state[SDL_GetScancodeFromKey(_key, NULL)])
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool InputManager::GetKeyDown(SDL_Keycode _key)
{
	const bool* state = SDL_GetKeyboardState(NULL);
	if (std::find(m_vDownKeys.begin(), m_vDownKeys.end(), SDL_GetScancodeFromKey(_key, NULL)) != m_vDownKeys.end())
	{
		if (std::find(m_vOldDownKeys.begin(), m_vOldDownKeys.end(), SDL_GetScancodeFromKey(_key, NULL)) == m_vOldDownKeys.end())
		{
			m_vOldDownKeys.push_back(SDL_GetScancodeFromKey(_key, NULL));
			return true;
		}
	}

	return false;
}

bool InputManager::GetKeyUp(SDL_Keycode _key)
{
	for (int i = 0; i < m_vUpKeys.size(); i++)
	{
		if (m_vUpKeys.at(i) == SDL_GetScancodeFromKey(_key, NULL))
		{
			return true;
		}
	}

	return false;
}

bool InputManager::GetMouseButton(int _button)
{
	if (SDL_GetMouseState(&m_fMouseX, &m_fMouseY) & SDL_BUTTON_MASK(_button))
	{
		return true;
	}

	return false;
}

bool InputManager::GetMouseButtonDown(int _button)
{
	for (int i = 0; i < m_vDownMouseButtons.size(); i++)
	{
		if (m_vDownMouseButtons.at(i) == _button)
		{
			if (std::find(m_vOldMouseButtons.begin(), m_vOldMouseButtons.end(), _button) == m_vOldMouseButtons.end())
			{
				return true;
			}
		}
	}

	return false;
}

bool InputManager::GetMouseButtonUp(int _button)
{
	for (int i = 0; i < m_vUpMouseButtons.size(); i++)
	{
		if (m_vUpMouseButtons.at(i) == _button)
		{
			return true;
		}
	}

	return false;
}

void InputManager::GetMousePosition()
{
	SDL_GetMouseState(&m_fMouseX, &m_fMouseY);
}

void InputManager::ClearFrameInputs()
{
	m_vOldMouseButtons = m_vDownMouseButtons;

	m_vDownKeys.clear();
	m_vUpKeys.clear();
	m_vDownMouseButtons.clear();
	m_vUpMouseButtons.clear();
}