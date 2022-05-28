#pragma once

#include <glm/glm.hpp>

#include <material.h>
#include <entity.h>

namespace Renderer {

	static void draw_model(uint32_t guid, glm::mat4 transform, RawModel* model, ExampleMaterial* material, const EnvironmentSettings& settings)
	{
		model->bind();
		model->draw();
		model->unbind();
	}
};