#pragma once

#include <string>

#include <imgui.h>
#include <ImGuizmo.h>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "shader.h"
#include "model.h"
#include "material.h"

struct NameComponent
{
	std::string name = "Entity";

	NameComponent() = default;
	NameComponent(const char* n) : name(n) {};
	NameComponent(const std::string& n) : name(n) {};

	static void DrawUI(const NameComponent& component)
	{
		ImGui::Text("Name: %s", component.name.c_str());
	}
};

struct TransformComponent
{
	glm::mat4 transform = glm::mat4(1.0f);

	TransformComponent() = default;
	TransformComponent(glm::mat4 t) : transform(t) {};

	static void DrawUI(const TransformComponent& component)
	{
		if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
		{
			glm::vec3 translation;
			glm::vec3 rotation;
			glm::vec3 scale;
			ImGuizmo::DecomposeMatrixToComponents((float*) &component.transform[0], &translation.x, &rotation.x, &scale.x);
			ImGui::InputFloat3("Translation", &translation.x);
			ImGui::InputFloat3("Rotation", &rotation.x);
			ImGui::InputFloat3("Scale", &scale.x);
			ImGuizmo::RecomposeMatrixFromComponents(&translation.x, &rotation.x, &scale.x, (float*)&component.transform[0]);
		}
	}
};

struct ModelRendererComponent
{
	RawModel* model = nullptr;

	ModelRendererComponent() = default;
	ModelRendererComponent(RawModel* m) : model(m) {};

	static void DrawUI(const ModelRendererComponent& component)
	{
		if (ImGui::CollapsingHeader("Model Renderer", ImGuiTreeNodeFlags_DefaultOpen))
		{

		}
	}
};

/**
* TODO: This does not need a RawModel(?)
*/
struct QuadRendererComponent
{
	RawModel* model = nullptr;

	QuadRendererComponent() = default;
	QuadRendererComponent(RawModel* m) : model(m) {};

	static void DrawUI(const QuadRendererComponent& component)
	{
		if (ImGui::CollapsingHeader("Quad Renderer", ImGuiTreeNodeFlags_DefaultOpen))
		{
			
		}
	}
};

struct MaterialComponent
{
	ExampleMaterial material;

	MaterialComponent() = default;

	static void DrawUI(MaterialComponent& component)
	{
		if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
		{
			component.material.DrawUI();
		}
	}
};