#pragma once
#ifndef UTILS_H
#define UTILS_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "config.h"

glm::vec3 RGBColor(float R, float G, float B);

unsigned int getQuadVAO();

void renderQuad();

void render3DQuad();

unsigned int getNoiseTexture();

unsigned int gaussianBlur(unsigned int sourceTex, float sigma, float samples,
                          float scale, int amount = 5);

void copyTexture2D(unsigned int source, unsigned int target);

#endif