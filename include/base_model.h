#pragma once

#include "model.h"
#include "material.h"

struct BaseModel {
  RawModel* raw_model;
  glm::mat4 transform;

  uint32_t material_index = 0;
  Material* materials[2];
};