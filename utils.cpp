#include "utils.h"
#include "shader_s.h"
#include <iostream>
#include <vector>
#include <random>
using namespace std;

glm::vec3 RGBColor(float R, float G, float B) {
  return glm::vec3(R / 255, G / 255, B / 255);
}

unsigned int getQuadVAO() {
  float quadVertices[] = {// vertex attributes for a quad that fills the entire
                          // screen in Normalized Device Coordinates.
                          // positions   // texCoords
                          -1.0f, 1.0f, 0.0f, 1.0f,  -1.0f, -1.0f,
                          0.0f,  0.0f, 1.0f, -1.0f, 1.0f,  0.0f,

                          -1.0f, 1.0f, 0.0f, 1.0f,  1.0f,  -1.0f,
                          1.0f,  0.0f, 1.0f, 1.0f,  1.0f,  1.0f};
  unsigned int VAO, VBO;
  glGenBuffers(1, &VBO);
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices,
               GL_STATIC_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                        (void *)(2 * sizeof(float)));
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glBindVertexArray(0);
  return VAO;
}

unsigned int getQuad3DVAO() {
  cout << "Create Quad VAO" << endl;
  unsigned int quadVAO, quadVBO;
  glm::vec3 pos1(-1.0f, 1.0f, 0.0f);
  glm::vec3 pos2(-1.0f, -1.0f, 0.0f);
  glm::vec3 pos3(1.0f, -1.0f, 0.0f);
  glm::vec3 pos4(1.0f, 1.0f, 0.0f);
  // texture coordinates
  glm::vec2 uv1(0.0f, 1.0f);
  glm::vec2 uv2(0.0f, 0.0f);
  glm::vec2 uv3(1.0f, 0.0f);
  glm::vec2 uv4(1.0f, 1.0f);
  // normal vector
  glm::vec3 nm(0.0f, 0.0f, 1.0f);

  // calculate tangent/bitangent vectors of both triangles
  glm::vec3 tangent1, bitangent1;
  glm::vec3 tangent2, bitangent2;
  // triangle 1
  // ----------
  glm::vec3 edge1 = pos2 - pos1;
  glm::vec3 edge2 = pos3 - pos1;
  glm::vec2 deltaUV1 = uv2 - uv1;
  glm::vec2 deltaUV2 = uv3 - uv1;

  float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

  tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
  tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
  tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
  tangent1 = glm::normalize(tangent1);

  bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
  bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
  bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
  bitangent1 = glm::normalize(bitangent1);

  // triangle 2
  // ----------
  edge1 = pos3 - pos1;
  edge2 = pos4 - pos1;
  deltaUV1 = uv3 - uv1;
  deltaUV2 = uv4 - uv1;

  f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

  tangent2.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
  tangent2.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
  tangent2.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
  tangent2 = glm::normalize(tangent2);

  bitangent2.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
  bitangent2.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
  bitangent2.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
  bitangent2 = glm::normalize(bitangent2);

  float quadVertices[] = {
      // positions            // normal         // texcoords  // tangent //
      // bitangent
      pos1.x,       pos1.y,       pos1.z,       nm.x,         nm.y,
      nm.z,         uv1.x,        uv1.y,        tangent1.x,   tangent1.y,
      tangent1.z,   bitangent1.x, bitangent1.y, bitangent1.z, pos2.x,
      pos2.y,       pos2.z,       nm.x,         nm.y,         nm.z,
      uv2.x,        uv2.y,        tangent1.x,   tangent1.y,   tangent1.z,
      bitangent1.x, bitangent1.y, bitangent1.z, pos3.x,       pos3.y,
      pos3.z,       nm.x,         nm.y,         nm.z,         uv3.x,
      uv3.y,        tangent1.x,   tangent1.y,   tangent1.z,   bitangent1.x,
      bitangent1.y, bitangent1.z,

      pos1.x,       pos1.y,       pos1.z,       nm.x,         nm.y,
      nm.z,         uv1.x,        uv1.y,        tangent2.x,   tangent2.y,
      tangent2.z,   bitangent2.x, bitangent2.y, bitangent2.z, pos3.x,
      pos3.y,       pos3.z,       nm.x,         nm.y,         nm.z,
      uv3.x,        uv3.y,        tangent2.x,   tangent2.y,   tangent2.z,
      bitangent2.x, bitangent2.y, bitangent2.z, pos4.x,       pos4.y,
      pos4.z,       nm.x,         nm.y,         nm.z,         uv4.x,
      uv4.y,        tangent2.x,   tangent2.y,   tangent2.z,   bitangent2.x,
      bitangent2.y, bitangent2.z};
  // configure plane VAO
  glGenVertexArrays(1, &quadVAO);
  glGenBuffers(1, &quadVBO);
  glBindVertexArray(quadVAO);
  glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices,
               GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float),
                        (void *)0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 14 * sizeof(float),
                        (void *)(6 * sizeof(float)));
  glEnableVertexAttribArray(3);
  glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float),
                        (void *)(8 * sizeof(float)));
  glEnableVertexAttribArray(4);
  glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float),
                        (void *)(11 * sizeof(float)));
  return quadVAO;
}

void renderQuad() {
  static unsigned int quadVAO = getQuadVAO();
  glBindVertexArray(quadVAO);
  glDrawArrays(GL_TRIANGLES, 0, 6);
}

void render3DQuad() {
  static unsigned int quadVAO = getQuad3DVAO();
  glBindVertexArray(quadVAO);
  glDrawArrays(GL_TRIANGLES, 0, 6);
}

unsigned int getNoiseTexture() {
  static bool inited = false;
  static unsigned noiseTexture = 0;
  static std::vector<glm::vec3> noiseVec;
  if (!inited) {
    inited = true;
    std::uniform_real_distribution<float> randomFloats(
        0.0, 1.0); // random floats between [0.0, 1.0]
    std::default_random_engine generator;
    for (unsigned int i = 0; i < 16; i++) {
      glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0,
                      randomFloats(generator) * 2.0 - 1.0, 0.0f);
      noiseVec.push_back(noise);
    }

    glGenTextures(1, &noiseTexture);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 4, 4, 0, GL_RGB, GL_FLOAT,
                 &noiseVec[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  }
  return noiseTexture;
}

unsigned int gaussianBlur(unsigned int sourceTex, float sigma, float samples,
                          float scale, int amount) {
  static Shader shaderBlur("gaussianBlur.vs", "gaussianBlur.fs");
  static bool init = true;
  static unsigned int pingpongFBO[2];
  static unsigned int pingpongBuffer[2];
  if (init) {
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongBuffer);
    for (unsigned int i = 0; i < 2; i++) {
      glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
      glBindTexture(GL_TEXTURE_2D, pingpongBuffer[i]);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, WINDOW_WIDTH, WINDOW_HEIGHT, 0,
                   GL_RGBA, GL_FLOAT, NULL);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_2D, pingpongBuffer[i], 0);
    }
    init = false;
  }

  bool horizontal = true, first_iteration = true;
  amount *= 2;
  shaderBlur.use();
  shaderBlur.setFloat("sigma", sigma);
  shaderBlur.setFloat("scale", scale);
  shaderBlur.setInt("samples", samples);
  for (unsigned int i = 0; i < amount; i++) {
    glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
    shaderBlur.setInt("horizontal", horizontal);
    glBindTexture(GL_TEXTURE_2D,
                  first_iteration ? sourceTex : pingpongBuffer[!horizontal]);
    renderQuad();
    horizontal = !horizontal;
    if (first_iteration)
      first_iteration = false;
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  return pingpongBuffer[!horizontal];
}

void copyTexture2D(unsigned int source, unsigned int target) {
  static bool inited = false;
  static unsigned int copyFBO;
  static Shader copyShader("copy.vs", "copy.fs");
  if (!inited) {
    inited = true;
    glGenFramebuffers(1, &copyFBO);
  }
  bool blend = glIsEnabled(GL_BLEND);
  if (blend)
    glDisable(GL_BLEND);
  glBindFramebuffer(GL_FRAMEBUFFER, copyFBO);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         target, 0);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, source);
  copyShader.use();
  renderQuad();
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  if (blend)
    glEnable(GL_BLEND);
}