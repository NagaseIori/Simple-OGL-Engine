#ifndef LIGHT_H
#define LIGHT_H

#include "shader_s.h"
#include "utils.h"
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
using namespace std;

unsigned int getLightVAO();

enum LightType { POINT, DIRECTIONAL, SPOTLIGHT };
const unsigned int SHADOW_WIDTH = 8192, SHADOW_HEIGHT = 8192;

class Light;
class Lights;

class Light {
public:
  glm::vec3 color;
  glm::vec3 position;
  glm::vec3 scale;
  glm::vec3 direction;
  glm::vec3 diffuse;
  glm::vec3 ambient;
  glm::vec3 specular;
  float diffuseRatio = 0.5;
  float ambientRatio = 0.2;
  float specularRatio = 1.0;

  float cutOff;
  float outerCutOff;
  float constant = 1.0;
  float linear = 0.14;
  float quadratic = 0.07;
  unsigned int shadowMap, shadowFBO;
  LightType type;
  glm::mat4 lightProjection;
  glm::mat4 model;

  void setDirectionalProjection(float left, float right, float bottom,
                                float top, float nearPlane, float farPlane) {
    if (type != DIRECTIONAL) {
      cout << "::Warning:: Setting directional projection to a non-directional "
              "light!"
           << endl;
    }
    lightProjection = glm::ortho(left, right, bottom, top, nearPlane, farPlane);
    updateSpaceMatrix();
  }

  void setSpotlightProjection(float FOV, float aspect, float nearPlane,
                              float farPlane) {
    if (type != SPOTLIGHT) {
      cout << "::Warning:: Setting spotlight projection to a non-spotlight "
              "light!"
           << endl;
    }
    lightProjection = glm::perspective(FOV, aspect, nearPlane, farPlane);
    updateSpaceMatrix();
  }

  void setPointProjection(float nearPlane, float farPlane) {
    if (type != POINT) {
      cout << "::Warning:: Setting spotlight projection to a non-spotlight "
              "light!"
           << endl;
    }
    float aspect = (float)shadowWidth / (float)shadowHeight;
    lightProjection =
        glm::perspective(glm::radians(90.0f), aspect, nearPlane, farPlane);
    this->farPlane = farPlane;
    updateSpaceMatrix();
  }

  void setAttenuation(float constant, float linear, float quadratic) {
    this->constant = constant;
    this->linear = linear;
    this->quadratic = quadratic;
    resetColor();
  }

  void setColorRatio(float diffuse, float ambient, float specular) {
    diffuseRatio = diffuse;
    ambientRatio = ambient;
    specularRatio = specular;
  }

  void resetColor() {
    diffuse = color * diffuseRatio;
    ambient = diffuse * ambientRatio;
    specular = color * specularRatio;
  }

  void setColor(glm::vec3 color) {
    this->color = color;
    resetColor();
  }

  void setType(LightType type) {
    if (initialized) {
      cout << "::ERROR:: Light type cannot be changed after initialization."
           << endl;
      exit(-1);
    }
    this->type = type;
  }

  template <typename F>
  void updateShadowMap(Shader &depthShader, F renderScene) {
    if (!shadowCast)
      return;
    glViewport(0, 0, shadowWidth, shadowHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);

    glClear(GL_DEPTH_BUFFER_BIT);
    depthShader.use();
    if (type != POINT) {
      depthShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
    } else {
      for (int i = 0; i < 6; i++)
        depthShader.setMat4("shadowMatrices[" + std::to_string(i) + "]",
                            shadowTransforms[i]);
      depthShader.setVec3("lightPos", position);
      depthShader.setFloat("far_plane", farPlane);
      depthShader.setMat4("model", model);
    }
    renderScene(depthShader);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
  }

  Light() { scale = glm::vec3(1.0); }

  void setPosition(glm::vec3 pos) {
    position = pos;
    updateModelMatrix();
    if (shadowCast)
      updateSpaceMatrix();
  }

  void setDirection(glm::vec3 dir) {
    direction = dir;
    updateModelMatrix();
    if (shadowCast)
      updateSpaceMatrix();
  }

  void setScale(glm::vec3 scl) {
    scale = scl;
    updateModelMatrix();
  }

  void setMapResolution(unsigned int width, unsigned int height) {
    if (initialized) {
      cout << "::ERROR:: You cannot set shadow map resolution after "
              "initialization"
           << endl;
      return;
    }
    shadowWidth = width;
    shadowHeight = height;
  }

private:
  friend class Lights;
  glm::mat4 lightSpaceMatrix;
  std::vector<glm::mat4> shadowTransforms;
  bool initialized = false;
  bool shadowCast = false;
  float farPlane;
  unsigned int shadowWidth = SHADOW_WIDTH, shadowHeight = SHADOW_HEIGHT;

  void updateSpaceMatrix();
  void updateModelMatrix();
  void setupShadowMap();
  void setupShader(int, int, Shader &);
  void initialize();
};

class Lights {
private:
  Shader depthShader, lightShader, lightSourceShader;
  Shader pointDepthShader;
  unsigned int lightVAO;

public:
  vector<Light> lights;

  void addLight(Light light) {
    light.initialize();
    lights.push_back(light);
  }
  template <typename F> void updateShadowMap(F renderScene) {
    for (auto &light : lights)
      light.updateShadowMap(depthShader, renderScene);
  }
  template <typename F>
  void render(glm::vec3 viewPos, F renderScene, void transformation(Shader &)) {
    // Setup shadowmap textures & shader
    lightShader.use();
    lightShader.setVec3("viewPos", viewPos);
    lightShader.setFloat("material.shininess", 2.0f);
    lightShader.setInt("lightCount", lights.size());
    for (int i = 0; i < lights.size(); i++) {
      glActiveTexture(GL_TEXTURE10 + i);
      glBindTexture(lights[i].type != POINT ? GL_TEXTURE_2D
                                            : GL_TEXTURE_CUBE_MAP,
                    lights[i].shadowMap);
      lights[i].setupShader(i, 10 + i, lightShader);
    }

    // Render
    renderScene(lightShader);

    // Render point light cubes
    for (auto &light : lights)
      if (light.type == POINT) {
        lightSourceShader.use();
        glBindVertexArray(lightVAO);
        transformation(lightSourceShader);
        lightSourceShader.setMat4("model", light.model);
        lightSourceShader.setVec3("lightColor", light.color);
        glDrawArrays(GL_TRIANGLES, 0, 36);
      }
  }

  Lights()
      : depthShader("simpleDepthShader.vs", "simpleDepthShader.fs"),
        lightShader("light.vs", "light_v2.fs"),
        lightSourceShader("light.vs", "light_source.fs"),
        pointDepthShader("point_light_depth.vs", "point_light_depth.fs",
                         "point_light_depth.gs") {
    lightVAO = getLightVAO();
  }
};

#endif