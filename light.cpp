#include "light.h"
using namespace std;

void Light::updateSpaceMatrix() {
  switch (type) {
  case DIRECTIONAL:
  case SPOTLIGHT:
    glm::mat4 lightView =
        glm::lookAt(position, position + direction, glm::vec3(0.f, 1.f, 0.f));
    lightSpaceMatrix = lightProjection * lightView;
    shadowCast = true;
    break;
  default:
    break;
  }
}

void Light::setupShadowMap() {
  unsigned int depthMapFBO;
  glGenFramebuffers(1, &depthMapFBO);
  unsigned int depthMap;
  glGenTextures(1, &depthMap);
  glBindTexture(GL_TEXTURE_2D, depthMap);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowWidth,
               shadowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                         depthMap, 0);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE); // not going to draw any color data
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  shadowFBO = depthMapFBO;
  shadowMap = depthMap;
}

void Light::setupShader(int index, int shadowMapIndex, Shader &shader) {
  if (!initialized) {
    cout << "::ERROR:: Light not initialized!" << endl;
    return;
  }

  std::string light_prefix =
      std::string("lights[") + std::to_string(index) + "].";
  auto elestr = [&](std::string element) { return light_prefix + element; };

  shader.setVec3(elestr("position"), position);
  shader.setVec3(elestr("direction"), direction);
  shader.setFloat(elestr("cutOff"), cutOff);
  shader.setFloat(elestr("outerCutOff"), outerCutOff);
  shader.setVec3(elestr("ambient"), ambient);
  shader.setVec3(elestr("diffuse"), diffuse);
  shader.setVec3(elestr("specular"), specular);
  shader.setFloat(elestr("constant"), constant);
  shader.setFloat(elestr("linear"), linear);
  shader.setFloat(elestr("quadratic"), quadratic);
  shader.setInt(elestr("type"), type);
  shader.setInt(elestr("shadowCast"), shadowCast);
  shader.setInt(elestr("shadowMap"), shadowMapIndex);
  shader.setMat4(elestr("lightSpace"), lightSpaceMatrix);

}

void Light::initialize() {
  setupShadowMap();
  initialized = true;
}

void Light::updateModelMatrix() {
  glm::mat4 model(1.0f);
  model = glm::translate(model, position);
  model = glm::scale(model, scale);
  this->model = model;
}
