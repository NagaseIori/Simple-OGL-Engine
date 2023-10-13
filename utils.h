#ifndef UTILS_H
#define UTILS_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

glm::vec3 RGBColor(float R, float G, float B) {
  return glm::vec3(R/255, G/255, B/255);
}

#endif