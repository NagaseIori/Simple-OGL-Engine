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
extern bool ssaoEnabled;

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
  float radius;
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
    resetColor();
  }

  void resetColor() {
    diffuse = color * diffuseRatio;
    ambient = diffuse * ambientRatio;
    specular = color * specularRatio;
    caculateRadius();
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
    if (!shadowCast || !shadowEnabled)
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
    if (shadowMapInited) {
      cout << "::ERROR:: You cannot set shadow map resolution after "
              "shadowMap's initialization"
           << endl;
      return;
    }
    shadowWidth = width;
    shadowHeight = height;
  }

  void toggleShadow(bool enabled) { shadowEnabled = enabled; }

  void updateMatrix() {
    direction = glm::normalize(direction);
    updateModelMatrix();
    if (shadowCast)
      updateSpaceMatrix();
  }

private:
  friend class Lights;
  glm::mat4 lightSpaceMatrix;
  std::vector<glm::mat4> shadowTransforms;
  bool initialized = false;
  bool shadowMapInited = false;
  bool shadowCast = false;
  bool shadowEnabled = true;
  float farPlane;
  unsigned int shadowWidth = SHADOW_WIDTH, shadowHeight = SHADOW_HEIGHT;

  void updateSpaceMatrix();
  void updateModelMatrix();
  void caculateRadius();
  float getLightMax();
  void setupShadowMap();
  void setupShader(int, int, Shader &);
  void initialize();
};

class Lights {
private:
  Shader depthShader;
  Shader lightSourceShader;
  Shader pointDepthShader;
  Shader gBufferShader;
  Shader lightPassShader;
  Shader lightFinalShader;
  Shader ssaoShader;
  Shader ssaoBlurShader;
  void setupLightFBO();
  void setupGBuffer();
  void setupSSAO();
  std::vector<glm::vec3> ssaoKernel;
  void sendSamplesToShader(Shader &shader);

public:
  unsigned int lightVAO, lightFBO, gLightAlbedo, gLightSpec;
  unsigned int gBuffer;
  unsigned int gPosition, gNormal, gAlbedo, gSpec, rboDepth;
  unsigned int ssaoFBO, ssaoMap, noiseTex, ssaoBlurFBO, ssaoMapBlurred;
  vector<Light> lights;

  void addLight(Light light) {
    light.initialize();
    lights.push_back(light);
  }
  template <typename F> void updateShadowMap(F renderScene) {
    for (auto &light : lights)
      if (light.type != POINT)
        light.updateShadowMap(depthShader, renderScene);
      else
        light.updateShadowMap(pointDepthShader, renderScene);
  }
  template <typename F>
  void render(glm::vec3 viewPos, F renderScene, void transformation(Shader &),
              unsigned int targetFBO) {
    // Setup shadowmap textures & shader
    lightPassShader.use();
    lightPassShader.setVec3("viewPos", viewPos);
    lightPassShader.setFloat("shininess", 32.0f);

    // First-pass: Geometry info -> gBuffer
    // glDisable(GL_BLEND); // Disable blend for g-buffer
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    glClearColor(0.0, 0.0, 0.0,
                 1.0); // keep it black so it doesn't leak into g-buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gBufferShader.use();
    gBufferShader.setVec3("viewPos", viewPos);
    transformation(gBufferShader);
    renderScene(gBufferShader);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // glEnable(GL_BLEND); // Re-enable blend

    // SSAO Render
    glDisable(GL_BLEND);
    if (ssaoEnabled) {
      glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
      glClear(GL_COLOR_BUFFER_BIT);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, gPosition);
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, gNormal);
      glActiveTexture(GL_TEXTURE2);
      glBindTexture(GL_TEXTURE_2D, noiseTex);
      ssaoShader.use();
      sendSamplesToShader(ssaoShader);
      transformation(ssaoShader);
      renderQuad();
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // SSAO Blurred
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
    glClear(GL_COLOR_BUFFER_BIT);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ssaoMap);
    ssaoBlurShader.use();
    ssaoBlurShader.setInt("ssaoEnabled", ssaoEnabled);
    renderQuad();
    glEnable(GL_BLEND);

    // Second-pass: Light info -> lightMap
    glBindFramebuffer(GL_FRAMEBUFFER, lightFBO);
    glClear(GL_COLOR_BUFFER_BIT);
    glBlendFunc(GL_ONE, GL_ONE); // set blendmode to add
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, ssaoMapBlurred);
    // also send light relevant uniforms
    lightPassShader.use();
    for (int i = 0; i < lights.size(); i++) {
      glActiveTexture(GL_TEXTURE10 + (lights[i].type == POINT ? 1 : 0));
      glBindTexture(lights[i].type != POINT ? GL_TEXTURE_2D
                                            : GL_TEXTURE_CUBE_MAP,
                    lights[i].shadowMap);
      lights[i].setupShader(i, 10 + (lights[i].type == POINT ? 1 : 0),
                            lightPassShader);
      renderQuad();
    }
    glBlendFunc(GL_SRC_ALPHA,
                GL_ONE_MINUS_SRC_ALPHA); // reset blendmode to normal
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Third-pass: lightMap -> target
    lightFinalShader.use();
    glBindFramebuffer(GL_FRAMEBUFFER, targetFBO);
    glClear(GL_COLOR_BUFFER_BIT);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gAlbedo);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gSpec);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gLightAlbedo);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, gLightSpec);
    renderQuad();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Copy Depth Buffer
    glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER,
                      targetFBO); // write to target framebuffer
    glBlitFramebuffer(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, 0, WINDOW_WIDTH,
                      WINDOW_HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Render point light cubes
    glBindFramebuffer(GL_FRAMEBUFFER, targetFBO);
    glDisable(GL_CULL_FACE);
    for (auto &light : lights)
      if (light.type == POINT) {
        lightSourceShader.use();
        glBindVertexArray(lightVAO);
        transformation(lightSourceShader);
        lightSourceShader.setMat4("model", light.model);
        lightSourceShader.setVec3("lightColor", light.color);
        glDrawArrays(GL_TRIANGLES, 0, 36);
      }
    // glEnable(GL_CULL_FACE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  Lights()
      : depthShader("simpleDepthShader.vs", "simpleDepthShader.fs"),
        lightSourceShader("light.vs", "light_source.fs"),
        pointDepthShader("point_light_depth.vs", "point_light_depth.fs",
                         "point_light_depth.gs"),
        gBufferShader("gBufferShader.vs", "gBufferShader.fs"),
        lightPassShader("lightPassShader.vs", "lightPassShader.fs"),
        lightFinalShader("lightFinalShader.vs", "lightFinalShader.fs"),
        ssaoShader("SSAO.vs", "SSAO.fs"),
        ssaoBlurShader("ssaoBlur.vs", "ssaoBlur.fs") {
    lightVAO = getLightVAO();
    setupGBuffer();
    setupLightFBO();
    setupSSAO();

    // Configure shader
    lightFinalShader.use();
    lightFinalShader.setInt("gAlbedo", 0);
    lightFinalShader.setInt("gSpec", 1);
    lightFinalShader.setInt("gLightAlbedo", 2);
    lightFinalShader.setInt("gLightSpec", 3);

    ssaoShader.use();
    ssaoShader.setInt("gPosition", 0);
    ssaoShader.setInt("gNormal", 1);
    ssaoShader.setInt("texNoise", 2);
    ssaoShader.setVec2("windowSize", {WINDOW_WIDTH, WINDOW_HEIGHT});

    lightPassShader.use();
    lightPassShader.setInt("gPosition", 0);
    lightPassShader.setInt("gNormal", 1);
    lightPassShader.setInt("ssaoMap", 2);

    // Get noise texture
    noiseTex = getNoiseTexture();
  }
};

#endif