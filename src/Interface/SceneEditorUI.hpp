#pragma once

#include "ToolUI.h"
#include "../Renderers/Hardware/HardwareRenderer.h"

class SceneEditorUI : public ToolUI
{

	HardwareRenderer* renderer;

public:

	void SetRenderer(HardwareRenderer* renderer)
	{
		this->renderer = renderer;
	}

	void DoInterface() override
	{
		ImGui::Begin("Scene Editor", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

		bool sceneChanged = false;
		for(int i = 0; i < renderer->m_sceneObjects.size(); i++)
		{
			auto& obj = renderer->m_sceneObjects[i];
			if (ImGui::TreeNode((void*)(intptr_t)i, "Object %d: %s", i, obj.modelName.c_str()))
			{
				ImGui::Text("Model: %s", obj.modelName.c_str());

				if(ImGui::DragFloat3("Position", &obj.position.x, 0.1f))
					sceneChanged = true;

				if (ImGui::DragFloat3("Rotation", &obj.rotation.x, 0.1f))
					sceneChanged = true;

				if (ImGui::DragFloat3("Scale", &obj.scale.x, 0.1f))
					sceneChanged = true;

				ImGui::TreePop();
			}
		}

		ImGui::Dummy(ImVec2(0.0f, 10.0f));

		for(int i = 0; i < renderer->m_sceneMaterials.size(); i++)
		{
			auto& mat = renderer->m_sceneMaterials[i];
			if (ImGui::TreeNode((void*)(intptr_t)i, "Material %d", i))
			{
				if (ImGui::ColorEdit3("Albedo", &mat.albedo.r))
					sceneChanged = true;

				if (ImGui::DragFloat("Smoothness", &mat.smoothness, 0.01f, 0.0f, 1.0f))
					sceneChanged = true;

				if (ImGui::DragFloat("Fuzziness", &mat.fuzziness, 0.01f, 0.0f, 1.0f))
					sceneChanged = true;

				if (ImGui::DragFloat("Emission", &mat.emission, 0.01f, 0.0f, 100.0f))
					sceneChanged = true;


				ImGui::TreePop();
			}
		}

		ImGui::End();

		if (sceneChanged)
		{
			renderer->RebufferSceneData();
			renderer->SetRefreshAccumulation();
		}
	}
};