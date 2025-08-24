#pragma once

#include "glm/glm.hpp"

class HardwareRenderer;

class CameraController
{

public:

	float m_fMoveSpeed = 16.0f;

	float m_fSensitivity = 50.0f;
	glm::vec2 m_newMousePos = glm::vec2(), m_oldMousePos = glm::vec2();
	bool m_bFirstMouse = false;
	float pitch = 0.0f, yaw = -90.0f;

	bool m_bLockedMouse = false;

	CameraController();

	void Update(HardwareRenderer* renderer);
	void UpdateCameraRotation(HardwareRenderer* renderer);

	float GetSensitivity() const { return m_fSensitivity; }
	void SetSensitivity(float sensitivity) { m_fSensitivity = sensitivity; }

};