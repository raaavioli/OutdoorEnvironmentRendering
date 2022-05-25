#pragma once

#include <string>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <shader.h>
#include <model.h>

struct NameComponent
{
	NameComponent() = delete;
	NameComponent(const char* n) : name(n) {};
	NameComponent(std::string& n) : name(n) {};
	std::string name;
};

struct TransformComponent
{
	TransformComponent() : transform(glm::mat4(1.0f)) {};
	TransformComponent(glm::mat4 t) : transform(t) {};
	glm::mat4 transform;
};

struct ModelRendererComponent
{
	ModelRendererComponent() = delete;
	ModelRendererComponent(RawModel* m) : model(m) {};
	RawModel* model;
};

/**
* TODO: This does not need a RawModel(?)
*/
struct QuadRendererComponent
{
	QuadRendererComponent() = delete;
	QuadRendererComponent(RawModel* m) : model(m) {};
	RawModel* model;
};

struct MaterialComponent
{
	MaterialComponent() = delete;
	MaterialComponent(Material* m[2]) 
	{
		materials[0] = m[0];
		materials[1] = m[1];
	};

	uint32_t material_index = 0;
	Material* materials[2];
};