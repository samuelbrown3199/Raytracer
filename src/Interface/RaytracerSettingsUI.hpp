#pragma once

#include "ToolUI.h"
#include "../Renderers/Hardware/RaytracerTypes.h"
#include "../Renderers/Hardware/HardwareRenderer.h"
#include "../Renderers/Hardware/CameraController.h"

class RaytracerSettingsUI : public ToolUI
{

	RaytracePushConstants* pushConstants;
	CameraSettings* camera;
	HardwareRenderer* renderer;
	CameraController* cameraController;

public:

	void SetPushConstants(RaytracePushConstants* pushConstants)
	{
		this->pushConstants = pushConstants;
	}

	void SetCameraSettings(CameraSettings* camera)
	{
		this->camera = camera;
	}

	void SetRenderer(HardwareRenderer* renderer)
	{
		this->renderer = renderer;
	}

	void SetCameraController(CameraController* controller)
	{
		this->cameraController = controller;
	}

	void DoInterface() override
	{
		bool resetAccumulation = false;

		ImGui::Begin("Raytracer Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

		ImGui::SeparatorText("Camera Settings");
		ImGui::Dummy(ImVec2(0.0f, 5.0f));

		std::string cameraPosStr = FormatString("Camera Position: (%.2f, %.2f, %.2f)", camera->cameraPosition.x, camera->cameraPosition.y, camera->cameraPosition.z);
		ImGui::Text(cameraPosStr.c_str());
		ImGui::Dummy(ImVec2(0, 3));

		if (ImGui::DragFloat("Field of View", &camera->cameraFov, 0.1f, 1.0f, 179.0f))
			resetAccumulation = true;

		if (ImGui::DragFloat("Focus Distance", &camera->focusDistance, 0.1f, 0.1f, 1000.0f))
			resetAccumulation = true;

		if (ImGui::DragFloat("Defocus Angle", &camera->defocusAngle, 0.1f, 0.0f, 90.0f))
			resetAccumulation = true;

		ImGui::DragFloat("Camera Speed", &cameraController->m_fMoveSpeed, 0.1f, 1.0f, 100.0f, "%.1f");

		ImGui::Dummy(ImVec2(0.0f, 5.0f));
		ImGui::SeparatorText("Render Settings");
		ImGui::Dummy(ImVec2(0.0f, 5.0f));

		if (ImGui::Combo("Render Mode", &pushConstants->renderMode, "Path Trace\0Show Bounding Boxes\0Show Depth\0Triangle Tests\0BVH Node Tests\0"))
			resetAccumulation = true;

		if (ImGui::DragInt("Rays Per Pixel", &pushConstants->raysPerPixel, 1, 1, 100))
			resetAccumulation = true;

		if (ImGui::DragInt("Max Bounces", &pushConstants->maxBounces, 1, 0, 100))
			resetAccumulation = true;

		static bool accumlateFrames = true;
		if (ImGui::Checkbox("Accumulate Frames", &accumlateFrames))
		{
			resetAccumulation = true;
		}
		pushConstants->accumulateFrames = accumlateFrames;

		ImGui::Text("Current Accumulated Frames: %d", pushConstants->frame);

		if (pushConstants->renderMode == 2)
		{
			if (ImGui::DragFloat("Depth Scale", &pushConstants->depthDebugScale, 0.1f, 0.1f, 100.0f))
				resetAccumulation = true;
		}

		if (pushConstants->renderMode == 3)
		{
			if (ImGui::DragInt("Max Triangle Tests", &pushConstants->triangleTestThreshold, 1, 1, 10000))
				resetAccumulation = true;
		}
		else if (pushConstants->renderMode == 4)
		{
			if (ImGui::DragInt("Max BVH Node Tests", &pushConstants->bvhNodeTestThreshold, 1, 1, 10000))
				resetAccumulation = true;
		}

		ImGui::Dummy(ImVec2(0.0f, 5.0f));

		static float sunlightColour[3] = { pushConstants->sunColour[0], pushConstants->sunColour[1], pushConstants->sunColour[2] };
		if (ImGui::ColorEdit3("Sunlight Colour", sunlightColour))
			resetAccumulation = true;

		pushConstants->sunColour = glm::vec3(sunlightColour[0], sunlightColour[1], sunlightColour[2]);

		static float sunlightDirection[3] = { pushConstants->sunDirection[0], pushConstants->sunDirection[1], pushConstants->sunDirection[2] };
		if (ImGui::DragFloat3("Sunlight Direction", sunlightDirection, 0.01f, -1.0f, 1.0f))
			resetAccumulation = true;

		pushConstants->sunDirection = glm::normalize(glm::vec3(sunlightDirection[0], sunlightDirection[1], sunlightDirection[2]));

		if (ImGui::DragFloat("Sunlight Intensity", &pushConstants->sunIntensity, 0.01f, 0.0f, 10.0f))
			resetAccumulation = true;

		ImGui::Dummy(ImVec2(0.0f, 5.0f));

		static int framesToRender = 100;

		ImGui::SeparatorText("Render Output Settings");
		ImGui::Dummy(ImVec2(0.0f, 5.0f));
		ImGui::Text("This will let the path tracer render and write the file out to disk.");
		ImGui::Text("This will block the main thread until the render is complete.");
		ImGui::Dummy(ImVec2(0.0f, 5.0f));
		if(ImGui::DragInt("Frames to Render", &framesToRender, 1, 1, 100000))
			renderer->SetRenderFrames(framesToRender);

		ImGui::Dummy(ImVec2(0.0f, 5.0f));
		if (ImGui::Button("Produce Render"))
		{
			renderer->SetDoRender();
		}

		if(resetAccumulation)
			renderer->SetRefreshAccumulation();

		ImGui::End();
	}
};