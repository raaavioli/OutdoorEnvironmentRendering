#pragma once

#include <glm/glm.hpp>

#include "shader.h"

struct EnvironmentSettings
{
	glm::mat4 camera_view_projection;
	glm::vec3 directional_light;
	glm::mat4 light_view_projection;
};

struct Material
{
	Shader* shader;

	virtual void bind(glm::mat4& transform, EnvironmentSettings& settings) = 0;
	virtual void unbind() = 0;
};

struct RawModelMaterial : Material
{
	void bind(glm::mat4& transform, EnvironmentSettings& settings) override;
	void unbind() override;

	glm::vec4 u_ModelColor;
	
	GLuint u_Texture;
	GLuint u_ShadowMap;	
};

struct RawModelFlatColorMaterial : Material
{
	void bind(glm::mat4& transform, EnvironmentSettings& settings) override;
	void unbind() override;

	glm::vec4 u_ModelColor;
};