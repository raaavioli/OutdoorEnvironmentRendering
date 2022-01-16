#pragma once

#include <glm/glm.hpp>

#include "base_model.h"

namespace Renderer {

	static void draw(BaseModel& model, const EnvironmentSettings& settings)
	{
		model.materials[model.material_index]->bind(model.transform, settings);
		model.raw_model->bind();
		model.raw_model->draw();
		model.raw_model->unbind();
	}
};