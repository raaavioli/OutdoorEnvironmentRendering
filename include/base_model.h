#pragma once

#include "model.h"
#include "material.h"

struct BaseModel {
  uint32_t id = 0;

  RawModel* raw_model;
  glm::mat4 transform;

  uint32_t material_index = 0;
  Material* materials[2];
};