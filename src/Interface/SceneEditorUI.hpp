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
			if (ImGui::TreeNode(("Object " + std::to_string(i) + "##obj").c_str(), "Object %d: %s", i, obj.modelName.c_str()))
			{
				ImGui::Text("Model: %s", obj.modelName.c_str());

				if(ImGui::DragFloat3("Position", &obj.position.x, 0.1f))
					sceneChanged = true;

				if (ImGui::DragFloat3("Rotation", &obj.rotation.x, 0.1f))
					sceneChanged = true;

				if (ImGui::DragFloat3("Scale", &obj.scale.x, 0.1f))
					sceneChanged = true;

				if (ImGui::BeginMenu("Material"))
				{
					for(int j = 0; j < renderer->m_sceneMaterials.size(); j++)
					{
						auto& mat = renderer->m_sceneMaterials[j];
						if (ImGui::MenuItem(("Material " + std::to_string(j) + "##matselect").c_str(), "", obj.materialIndex == j))
						{
							obj.materialIndex = j;
							sceneChanged = true;
						}
					}

					ImGui::EndMenu();
				}

				ImGui::TreePop();
			}
		}

		ImGui::Dummy(ImVec2(0.0f, 10.0f));

		for(int i = 0; i < renderer->m_sceneMaterials.size(); i++)
		{
			auto& mat = renderer->m_sceneMaterials[i];
			if (ImGui::TreeNode(("Material " + std::to_string(i) + "##mat").c_str(), "Material %d", i))
			{
				if (ImGui::ColorEdit3("Albedo", &mat.albedo.r))
					sceneChanged = true;

				if (ImGui::DragFloat("Smoothness", &mat.smoothness, 0.01f, 0.0f, 1.0f))
					sceneChanged = true;

				if (ImGui::DragFloat("Fuzziness", &mat.fuzziness, 0.01f, 0.0f, 1.0f))
					sceneChanged = true;

				if(ImGui::DragFloat("Refraction Index", &mat.refractiveIndex, 0.01f, 0.0f, 3.0f))
					sceneChanged = true;

				if(mat.refractiveIndex != 0.0f)
				{
					if (ImGui::ColorEdit3("Absorbtion", &mat.absorbtion.r))
						sceneChanged = true;
				}

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