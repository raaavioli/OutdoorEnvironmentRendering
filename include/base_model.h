#pragma once

#include "model.h"
#include "material.h"

struct BaseModel {
  RawModel* raw_model;
  Material* material;

  glm::mat4 transform;
};