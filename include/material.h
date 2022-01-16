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
	virtual void bind(glm::mat4& transform, const EnvironmentSettings& settings) = 0;
	virtual void unbind() = 0;
	
protected:
	Shader* shader;
};

struct RawModelMaterial : Material
{
	RawModelMaterial(Shader* shader, glm::vec4 model_color, GLuint texture, GLuint shadow_map) : u_ModelColor(model_color), u_Texture(texture), u_ShadowMap(shadow_map)
	{
		this->shader = shader;
	}

	void bind(glm::mat4& transform, const EnvironmentSettings& settings) override;
	void unbind() override;

	glm::vec4 u_ModelColor;
	
	GLuint u_Texture;
	GLuint u_ShadowMap;	
};

struct RawModelFlatColorMaterial : Material
{
	RawModelFlatColorMaterial(Shader* shader, glm::vec4 model_color) : u_ModelColor(model_color) 
	{
		this->shader = shader;
	}

	void bind(glm::mat4& transform, const EnvironmentSettings& settings) override;
	void unbind() override;

	glm::vec4 u_ModelColor;
};