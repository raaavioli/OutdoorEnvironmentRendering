#pragma once

#include <string>

#include <imgui.h>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <shader.h>
#include <model.h>


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
		if (ImGui::CollapsingHeader("Transform"))
		{
			glm::mat4 m = component.transform;
			ImGui::Text("%f\t%f\t%f\t%f", m[0][0], m[1][0], m[2][0], m[3][0]);
			ImGui::Text("%f\t%f\t%f\t%f", m[0][1], m[1][1], m[2][1], m[3][1]);
			ImGui::Text("%f\t%f\t%f\t%f", m[0][2], m[1][2], m[2][2], m[3][2]);
			ImGui::Text("%f\t%f\t%f\t%f", m[0][3], m[1][3], m[2][3], m[3][3]);
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
		if (ImGui::CollapsingHeader("Model Renderer"))
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
		if (ImGui::CollapsingHeader("Quad Renderer"))
		{

		}
	}
};

enum MaterialIndex
{
	SHADED = 0,
	FLAT = 1
};

struct MaterialComponent
{
	MaterialIndex active_material = MaterialIndex::SHADED;
	std::vector<Material*> materials;

	MaterialComponent() = default;

	static void DrawUI(const MaterialComponent& component)
	{
		if (ImGui::CollapsingHeader("Material"))
		{

		}
	}
};