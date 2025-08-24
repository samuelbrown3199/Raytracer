#include "CameraController.h"
#include "HardwareRenderer.h"

#include <SDL3/SDL.h>


CameraController::CameraController()
{
}

void CameraController::Update(HardwareRenderer* renderer)
{
	InputManager* inputManager = renderer->GetInputManager();
	float deltaTime = renderer->GetPerformanceStats()->GetDeltaT();

	UpdateCameraRotation(renderer);

	if (inputManager->GetKeyDown(SDLK_ESCAPE))
	{
		m_bLockedMouse = !m_bLockedMouse;
		renderer->GetWindow()->SetMouseMode(m_bLockedMouse);
	}

	if (!m_bLockedMouse)
		return;

	if (inputManager->GetKey(SDLK_W))
	{
		renderer->m_camera.cameraPosition += (m_fMoveSpeed) * renderer->m_camera.cameraLookDirection * deltaTime;
	}
	if (inputManager->GetKey(SDLK_S))
	{
		renderer->m_camera.cameraPosition -= (m_fMoveSpeed) * renderer->m_camera.cameraLookDirection * deltaTime;
	}
	if (inputManager->GetKey(SDLK_A))
	{
		glm::vec3 direction = glm::cross(renderer->m_camera.cameraLookDirection, glm::vec3(0, 1, 0));
		renderer->m_camera.cameraPosition -= (m_fMoveSpeed) * direction * deltaTime;
	}
	if (inputManager->GetKey(SDLK_D))
	{
		glm::vec3 direction = glm::cross(renderer->m_camera.cameraLookDirection, glm::vec3(0, 1, 0));
		renderer->m_camera.cameraPosition += (m_fMoveSpeed) * direction * deltaTime;
	}
	if (inputManager->GetKey(SDLK_SPACE))
	{
		renderer->m_camera.cameraPosition += (m_fMoveSpeed) * glm::vec3(0, 1, 0) * deltaTime;
	}
	if (inputManager->GetKey(SDLK_X))
	{
		renderer->m_camera.cameraPosition -= (m_fMoveSpeed)*glm::vec3(0, 1, 0) * deltaTime;
	}
}

void CameraController::UpdateCameraRotation(HardwareRenderer* renderer)
{
	InputManager* inputManager = renderer->GetInputManager();
	float deltaTime = renderer->GetPerformanceStats()->GetDeltaT();

	if (m_bLockedMouse)
	{
		float effectiveSensitivity = m_fSensitivity * deltaTime;
		float xoffset = inputManager->m_mouseDeltaX * effectiveSensitivity;
		float yoffset = -inputManager->m_mouseDeltaY * effectiveSensitivity;

		yaw += xoffset;
		pitch += yoffset;

		// Clamp pitch
		pitch = std::clamp(pitch, -89.0f, 89.0f);

		glm::vec3 front;
		front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		front.y = sin(glm::radians(pitch));
		front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

		renderer->m_camera.cameraLookDirection = glm::normalize(front);
	}
	else
	{
		m_bFirstMouse = true;
	}
}