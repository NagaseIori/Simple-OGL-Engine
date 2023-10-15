#ifndef OBJECT_H
#define OBJECT_H

#include "utils.h"
#include "model.h"

class Object {
public:
  glm::vec3 position;
  glm::vec3 scale;
  float angle;
  glm::vec3 axis;
  Model model;

  glm::mat4 getModelMatrix() {
    glm::mat4 mat(1.f);
    mat = glm::translate(mat, position + glm::vec3(0.f, GROUND_YOFFSET, 0.f));
    mat = glm::scale(mat, scale);
    if (angle != 0.0)
      mat = glm::rotate(mat, angle, axis);
    return mat;
  }

  Object(string const &path, bool flipUVs = true, bool gamma = true)
      : model(path, flipUVs, gamma) {
    position = {0.f, 0.f, 0.f};
    scale = {1.f, 1.f, 1.f};
    angle = 0;
    axis = {0.f, 0.f, 0.f};
  }

  void Draw(Shader &shader) {
    shader.setMat4("model", getModelMatrix());
    model.Draw(shader);
  }

  void setPosition(float x, float y, float z) { position = glm::vec3(x, y, z); }
  void setScale(float x, float y, float z) { scale = glm::vec3(x, y, z); }
  void setScale(float scale) { this->scale = glm::vec3(scale, scale, scale); }
  void setAngle(float angle) { this->angle = angle; }
  void setAxis(float x, float y, float z) { axis = glm::vec3(x, y, z); }
};

#endif