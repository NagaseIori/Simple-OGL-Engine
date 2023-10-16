#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "shader_s.h"
#include "model.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "camera.h"
#include "utils.h"
#include "light.h"
#include "object.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdio>
#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>

float visibility = 0.2;
float camera_height = 3.0;
float deltaTime = 0.0f; // Time between current frame and last frame
float lastFrame = 0.0f; // Time of last frame
float lastX = WINDOW_WIDTH / 2., lastY = WINDOW_HEIGHT / 2.;
float spotLightEnabled = 1.0;
bool depthDebug = false;
int debugSurface = 0;
const int DEBUG_SRUFACES = 10;
float exposure = 2.0;
bool pointShadow = POINT_SHADOW_START_ENABLED;
bool fxaaEnabled = true;
int dirLightStyle = 0;

float windowWidth = WINDOW_WIDTH, windowHeight = WINDOW_HEIGHT;

Camera mainCam(30.f, 30.f, 3.f, 0.f, 1.f, 0.f, -135.f, -45.f);

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
  windowWidth = width;
  windowHeight = height;
}

void scroll_callback(GLFWwindow *window, double xoff, double yoff) {
  if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
    debugSurface = debugSurface - yoff + DEBUG_SRUFACES + 1;
    debugSurface %= DEBUG_SRUFACES + 1;
  } else if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) {
    exposure += yoff * 0.25;
    exposure = glm::clamp(exposure, 0.1f, 100.f);
  } else {
    mainCam.ProcessMouseScroll(yoff);
  }
}
void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mods) {
  if (key == GLFW_KEY_KP_0 && action == GLFW_PRESS) {
    pointShadow = !pointShadow;
  }
  if (key == GLFW_KEY_KP_1 && action == GLFW_PRESS) {
    ssaoEnabled = !ssaoEnabled;
  }
  if (key == GLFW_KEY_KP_2 && action == GLFW_PRESS) {
    fxaaEnabled = !fxaaEnabled;
  }
  if (key == GLFW_KEY_KP_3 && action == GLFW_PRESS) {
    dirLightStyle++;
    dirLightStyle %= 2;
  }
}

void process_input(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, true);
  }
  float cameraSpeed = deltaTime;
  if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT))
    cameraSpeed *= 5;
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    mainCam.ProcessKeyboard(Camera_Movement::FORWARD, cameraSpeed);
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    mainCam.ProcessKeyboard(Camera_Movement::BACKWARD, cameraSpeed);
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    mainCam.ProcessKeyboard(Camera_Movement::LEFT, cameraSpeed);
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    mainCam.ProcessKeyboard(Camera_Movement::RIGHT, cameraSpeed);
  if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    spotLightEnabled = 1.0;
  else {
    spotLightEnabled = 0.0;
  }

  depthDebug = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
}

void shader_status_check(unsigned int shaderIndex) {
  int success;
  char infoLog[512];
  glGetShaderiv(shaderIndex, GL_COMPILE_STATUS, &success);

  if (!success) {
    glGetShaderInfoLog(shaderIndex, 512, NULL, infoLog);
    std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
              << infoLog << std::endl;
  }
}

void transformation(Shader &shd) {
  // Model transform
  glm::mat4 model(1.f);

  // View transform
  glm::mat4 view;
  view = mainCam.GetViewMatrix();

  // Projection transform
  glm::mat4 projection;
  projection =
      glm::perspective(glm::radians(mainCam.Zoom),
                       1.f * WINDOW_WIDTH / WINDOW_HEIGHT, 0.1f, FAR_PLANE);

  // Set transformation
  shd.setMat4("model", model);
  shd.setMat4("view", view);
  shd.setMat4("projection", projection);
}

#define AXIS_INF 10000.0f
unsigned int getAxisVAO() {
  float vertices[] = {-AXIS_INF, 0.f,       0.f,       1.f, 0.f, 0.f,
                      AXIS_INF,  0.f,       0.f,       1.f, 0.f, 0.f,
                      0.f,       -AXIS_INF, 0.f,       0.f, 1.f, 0.f,
                      0.f,       AXIS_INF,  0.f,       0.f, 1.f, 0.f,
                      0.f,       0.f,       -AXIS_INF, 0.f, 0.f, 1.f,
                      0.f,       0.f,       AXIS_INF,  0.f, 0.f, 1.f};
  unsigned int VAO, VBO;
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);
  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glBindVertexArray(0);
  return VAO;
}

const float boxVertices[] = {
    -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.5f,  -0.5f, -0.5f, 1.0f, 0.0f,
    0.5f,  0.5f,  -0.5f, 1.0f, 1.0f, 0.5f,  0.5f,  -0.5f, 1.0f, 1.0f,
    -0.5f, 0.5f,  -0.5f, 0.0f, 1.0f, -0.5f, -0.5f, -0.5f, 0.0f, 0.0f,

    -0.5f, -0.5f, 0.5f,  0.0f, 0.0f, 0.5f,  -0.5f, 0.5f,  1.0f, 0.0f,
    0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
    -0.5f, 0.5f,  0.5f,  0.0f, 1.0f, -0.5f, -0.5f, 0.5f,  0.0f, 0.0f,

    -0.5f, 0.5f,  0.5f,  1.0f, 0.0f, -0.5f, 0.5f,  -0.5f, 1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
    -0.5f, -0.5f, 0.5f,  0.0f, 0.0f, -0.5f, 0.5f,  0.5f,  1.0f, 0.0f,

    0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.5f,  0.5f,  -0.5f, 1.0f, 1.0f,
    0.5f,  -0.5f, -0.5f, 0.0f, 1.0f, 0.5f,  -0.5f, -0.5f, 0.0f, 1.0f,
    0.5f,  -0.5f, 0.5f,  0.0f, 0.0f, 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.5f,  -0.5f, -0.5f, 1.0f, 1.0f,
    0.5f,  -0.5f, 0.5f,  1.0f, 0.0f, 0.5f,  -0.5f, 0.5f,  1.0f, 0.0f,
    -0.5f, -0.5f, 0.5f,  0.0f, 0.0f, -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,

    -0.5f, 0.5f,  -0.5f, 0.0f, 1.0f, 0.5f,  0.5f,  -0.5f, 1.0f, 1.0f,
    0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f, 0.5f,  0.5f,  0.0f, 0.0f, -0.5f, 0.5f,  -0.5f, 0.0f, 1.0f};

void drawAxis(Shader &shader, unsigned int AxisVAO) {
  shader.use();
  transformation(shader);
  glBindVertexArray(AxisVAO);
  glDrawArrays(GL_LINES, 0, 6);
}

void setupScreenFBO(const unsigned int FBO, unsigned int &texture_o) {
  glBindFramebuffer(GL_FRAMEBUFFER, FBO);

  glGenTextures(1, &texture_o);
  glBindTexture(GL_TEXTURE_2D, texture_o);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, WINDOW_WIDTH, WINDOW_HEIGHT, 0,
               GL_RGB, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  unsigned int rbo;
  glGenRenderbuffers(1, &rbo);
  glBindRenderbuffer(GL_RENDERBUFFER, rbo);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, WINDOW_WIDTH,
                        WINDOW_HEIGHT);

  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                            GL_RENDERBUFFER, rbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         texture_o, 0);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!"
              << std::endl;
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void setupHdrFBO(unsigned int &hdrFBO, unsigned int &texture_out,
                 unsigned int &texture_bloom) {
  glGenFramebuffers(1, &hdrFBO);
  glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

  glGenTextures(1, &texture_out);
  glBindTexture(GL_TEXTURE_2D, texture_out);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, WINDOW_WIDTH, WINDOW_HEIGHT, 0,
               GL_RGB, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glGenTextures(1, &texture_bloom);
  glBindTexture(GL_TEXTURE_2D, texture_bloom);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, WINDOW_WIDTH, WINDOW_HEIGHT, 0,
               GL_RGB, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         texture_bloom, 0);

  unsigned int rbo;
  glGenRenderbuffers(1, &rbo);
  glBindRenderbuffer(GL_RENDERBUFFER, rbo);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, WINDOW_WIDTH,
                        WINDOW_HEIGHT);

  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                            GL_RENDERBUFFER, rbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         texture_out, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                         texture_bloom, 0);

  unsigned int attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
  glDrawBuffers(2, attachments);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!"
              << std::endl;
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

unsigned int loadCubemap(vector<std::string> faces) {
  stbi_set_flip_vertically_on_load(false);
  unsigned int textureID;
  glGenTextures(1, &textureID);
  glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
  int width, height, nrChannels;
  for (unsigned int i = 0; i < faces.size(); i++) {
    unsigned char *data =
        stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
    if (data) {
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_SRGB_ALPHA, width,
                   height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
      stbi_image_free(data);
    } else {
      std::cout << "Cubemap texture failed to load at path: " << faces[i]
                << std::endl;
      stbi_image_free(data);
    }
  }
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  // stbi_set_flip_vertically_on_load(true);

  return textureID;
}

unsigned int getSkyboxVAO() {
  float skyboxVertices[] = {
      // positions
      -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f,
      1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,

      -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f,
      -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,

      1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,
      1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,

      -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
      1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,

      -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,
      1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,

      -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f,
      1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f};
  unsigned int VAO, VBO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices,
               GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void *)0);
  glEnableVertexAttribArray(0);
  glBindVertexArray(0);
  return VAO;
}

void Scene1(GLFWwindow *window) {
  // Shaders compilation
  // ------------------
  Shader myShader("learn.vs", "learn.fs");
  Shader defaultShader("default.vs", "default.fs");

  // Random generate boxes and lights' positions
  // ------------------
  std::vector<glm::vec3> cubePositions, pointLightPositions;
  cubePositions.push_back({0.f, 0.f, 0.f});
  for (int i = 0; i < BAG_COUNT; i++) {
    cubePositions.push_back(
        {rand() % 100 - 50, rand() % 100 - 50, rand() % 100 - 50});
  }
  for (int i = 0; i < RANDOM_LIGHT_COUNT; i++)
    pointLightPositions.push_back(
        {rand() % 100 - 50, rand() % 100 - 50, rand() % 100 - 50});

  // Get needed VAOs
  // ------------------
  unsigned int AxisVAO = getAxisVAO();

  // Light Settings
  // ------------------
  unsigned int lightCubeVAO = getLightVAO();
  Lights lightSystem;
  Light spotLight, dirLight;

  /// SpotLight
  glm::vec3 spotLightOffset(1, -1.0f, 0);
#define LIGHT_FAR_PLANE 1200.f
  spotLight.setType(SPOTLIGHT);
  spotLight.cutOff = glm::cos(glm::radians(30.f));
  spotLight.outerCutOff = glm::cos(glm::radians(35.f));
  spotLight.setAttenuation(1, 0.07, 0.017);
  spotLight.setColorRatio(1, 0.01, 1);
  spotLight.setColor(glm::vec3(1.f));
  spotLight.setSpotlightProjection(glm::radians(35.f), 1, 0.1f,
                                   LIGHT_FAR_PLANE);
  spotLight.setMapResolution(2048, 2048);
  lightSystem.addLight(spotLight);

  /// Directional Light
  dirLight.setType(DIRECTIONAL);
  dirLight.setPosition({150.f, 300.f, 150.f});
  dirLight.setDirection({-0.5, -1, -0.15});
#define DIR_RANGE 400.f
  dirLight.setDirectionalProjection(-DIR_RANGE, DIR_RANGE, -DIR_RANGE,
                                    DIR_RANGE, 0.1f, LIGHT_FAR_PLANE);
  lightSystem.addLight(dirLight);
  /// Point Lights
  for (int i = 0; i < pointLightPositions.size(); i++) {
    Light pointLight;
    pointLight.setType(POINT);
    pointLight.setColorRatio(0.1, 0.001, 1);
    pointLight.setAttenuation(1, 0.35, 0.44);
    pointLight.setMapResolution(POINT_LIGHT_SHADOWMAP_RESOLUTION,
                                POINT_LIGHT_SHADOWMAP_RESOLUTION);
    if (i < RANDOM_LIGHT_WITH_SHADOW)
      pointLight.setPointProjection(0.1f, 400.f);
    lightSystem.addLight(pointLight);
  }

  // Load Models
  // ------------------
  Object modelBag("model/backpack/backpack.obj", false);
  Object modelGirl("model/girl/rin.obj");
  Object modelSponza("model/sponza/sponza.obj");
  Object modelPaimon("model/paimon/Paimon.obj");
  Object modelGaki("model/mesugaki/cute anime girl.obj");
  // Object modelTrain("model/train/scene.gltf");
  // Object modelBagH("model/backpackH/scene.gltf");

  // Setup Screen Framebuffer & Shader
  // ------------------
  unsigned int screenFBO, quadVAO, screenTex;
  glGenFramebuffers(1, &screenFBO);
  setupScreenFBO(screenFBO, screenTex);
  Shader screenShader("screen.vs", "screen.fs");
  Shader HDRShader("HDR.vs", "HDR.fs");
  quadVAO = getQuadVAO();

  // Setup HDR & Bloom FBO
  // ------------------
  Shader HDRBloomFinalShader("HDRBloomFinal.vs", "HDRBloomFinal.fs");
  HDRBloomFinalShader.use();
  HDRBloomFinalShader.setInt("scene", 0);
  HDRBloomFinalShader.setInt("bloomBlur", 1);
  unsigned int hdrFBO, hdrTex, bloomTex;
  setupHdrFBO(hdrFBO, hdrTex, bloomTex);

  // Load Cubemaps
  // ------------------
  // vector<std::string> faces{"night/right.png", "night/left.png",
  //                           "night/top.png",   "night/bottom.png",
  //                           "night/front.png", "night/back.png"};
  vector<std::string> faces{"skybox/right.jpg", "skybox/left.jpg",
                            "skybox/top.jpg",   "skybox/bottom.jpg",
                            "skybox/front.jpg", "skybox/back.jpg"};
  unsigned int cubemapTexture = loadCubemap(faces);
  Shader skyboxShader("skybox.vs", "skybox.fs");
  unsigned int skyboxVAO = getSkyboxVAO();

  // Load Parallax Surface
  // ------------------
  unsigned int brickDiffTex, brickNormalTex, brickDispTex;
  brickDiffTex = TextureFromFile("bricks2.jpg", "", true);
  brickNormalTex = TextureFromFile("bricks2_normal.jpg", "");
  brickDispTex = TextureFromFile("bricks2_disp.jpg", "");

  // Rendering Loop
  // ------------------
  glm::mat4 model(1.0f);

  // Depthmap setup
  // -----------------
  Shader depthDebugShader("depthShaderDebug.vs", "depthShaderDebug.fs");
  auto renderScene = [&](Shader &shader) {
    shader.use();
    transformation(shader);
    // Draw boxes
    // -------------
    for (unsigned int i = 0; i < cubePositions.size(); i++) {
      modelBag.position =
          cubePositions[i] +
          glm::vec3(0.f, sin(glfwGetTime() + i * 1.14) * 4.f + 100.f, 0.f);
      modelBag.angle = 20.0f * i + glfwGetTime();
      modelBag.axis = glm::vec3(1.0f, 0.3f, 0.5f);

      modelBag.Draw(shader);
      // modelMiku.Draw(shader);
    }

    // Draw Sponza
    modelSponza.setScale(0.1);
    modelSponza.Draw(shader);

    // Draw Girl
    modelGirl.setPosition(0, 0, -10);
    modelGirl.setScale(10);
    modelGirl.Draw(shader);

    // Draw Paimon
    modelGirl.setPosition(5, 0, -10);
    modelPaimon.Draw(shader);

    // Draw Gaki
    modelGaki.setPosition(-20, 0, -10);
    modelGaki.setScale(10);
    modelGaki.Draw(shader);

    // Draw backpack Highpoly ver
    // modelBagH.setPosition(20, 10, 10);
    // modelBagH.Draw(shader);

    // Draw Train
    // modelTrain.setPosition(20, 0, 0);
    // modelTrain.setScale(1);
    // modelTrain.Draw(shader);

    // Draw Parallax Test Surface
    glm::mat4 model(1.0f);
    model = glm::scale(model, glm::vec3(7.0f));
    model = glm::rotate(model, glm::radians(-90.f), glm::vec3(1.f, 0.f, 0.f));
    shader.setMat4("model", model);
    shader.setInt("material.diffuse[0]", 0);
    shader.setInt("material.normal[0]", 1);
    shader.setInt("material.height[0]", 2);
    shader.setInt("material.diffuse_c", 1);
    shader.setInt("material.normal_c", 1);
    shader.setInt("material.height_c", 1);
    shader.setInt("material.specular_c", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, brickDiffTex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, brickNormalTex);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, brickDispTex);
    render3DQuad();
  };
  while (!glfwWindowShouldClose(window)) {
    // Time Update
    // -----------------
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // Something else
    // -----------------

    // Pre-rendering
    // -----------------
    process_input(window);

    glBindFramebuffer(GL_FRAMEBUFFER, screenFBO);
    glEnable(GL_DEPTH_TEST);

    // Rendering
    // -----------------
    glClearColor(0.1f, 0.1f, 0.15f, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Fix Camera Y Position
    // -----------------
    // mainCam.Position.y = 1;

    // Setup Lights
    // -------------
    /// Direcitonal Light
    // -----------------

    switch (dirLightStyle) {
    case 0: // night
      lightSystem.lights[1].setColorRatio(0.3, 0.05, 0);
      lightSystem.lights[1].setColor(RGBColor(81, 104, 134));
      break;
    case 1: // sunset
      lightSystem.lights[1].setColorRatio(2.0, 0.15, 0);
      lightSystem.lights[1].setColor(RGBColor(239, 149, 149));
      break;
    default:
      break;
    }

    /// Spot Light
    // -----------------
    lightSystem.lights[0].setColor(RGBColor(255, 255, 255) * spotLightEnabled);
    lightSystem.lights[0].setSpotlightProjection(glm::radians(mainCam.Zoom),
                                                 1.0f, 0.1f, LIGHT_FAR_PLANE);
    // spotLight.setSpotlightProjection(glm::radians(35.f), 1, 0.1f,
    // LIGHT_FAR_PLANE);
    glm::vec3 spotLightPos = mainCam.Position + spotLightOffset;
    lightSystem.lights[0].setPosition(spotLightPos);
    lightSystem.lights[0].setDirection(mainCam.Front);
    lightSystem.lights[0].toggleShadow(spotLightEnabled);

    /// Point Lights
    // -----------------
    for (int i = 2; i < lightSystem.lights.size(); i++) {
      auto &light = lightSystem.lights[i];
      glm::vec3 lightPosOffset;
      lightPosOffset.x = cos(glfwGetTime() + i * 11.4) * 10.0;
      lightPosOffset.y = sin(glfwGetTime() * 3 + i * 11.4) * 3.0;
      lightPosOffset.z = sin(glfwGetTime() + i * 11.4) * 10.0;

      glm::vec3 lightColor(1.f);
      float time = glfwGetTime();
      lightColor.x = sin(time + i * 11.4) / 2 + 0.5;
      lightColor.y = cos(time + i * 11.4) / 2 + 0.5;
      lightColor.z = sin(time - i * 11.4) / 2 + 0.5;
      lightColor *= 3.f;

      light.setColor(lightColor);
      if (i - 2 < RANDOM_LIGHT_WITH_SHADOW) {
        light.setColorRatio(5, 0, 5);
        light.setPosition(lightPosOffset + glm::vec3(0.f, -30.f, 0.f));
        light.toggleShadow(pointShadow);
      } else
        light.setPosition(lightPosOffset + pointLightPositions[i - 2]);
    }
    // Render to depthmap
    lightSystem.updateShadowMap(renderScene);

    // Render Scene
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    lightSystem.render(mainCam.Position, renderScene, transformation,
                       screenFBO);
    glBindVertexArray(0);

    // draw axis and skybox as last
    glBindFramebuffer(GL_FRAMEBUFFER, screenFBO);
    // Draw Axis
    defaultShader.use();
    drawAxis(defaultShader, AxisVAO);
    glDepthFunc(GL_LEQUAL); // change depth function so depth test passes when
                            // values are equal to depth buffer's content
    skyboxShader.use();
    transformation(skyboxShader);
    glm::mat4 view = glm::mat4(glm::mat3(
        mainCam.GetViewMatrix())); // remove translation from the view matrix
    skyboxShader.setMat4("view", view);
    skyboxShader.setInt("skybox", 0);
    // skybox cube
    glBindVertexArray(skyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS); // set depth function back to default

    // Post-rendering
    // -----------------
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // back to default
    glClearColor(.2f, .2f, .2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, windowWidth, windowHeight);

    // HDR & Exposure & Bloom
    // -----------------
    // Extract Color
    glDisable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    HDRShader.use();
    HDRShader.setInt("screenTexture", 0);
    HDRShader.setFloat("threshold", BLOOM_THRESHOLD);
    HDRShader.setFloat("bound_ratio", BLOOM_BOUND_RATIO);
    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, screenTex);
    renderQuad();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Gaussian Blur
    unsigned int blurredTex =
        gaussianBlur(bloomTex, BLOOM_SIGMA, BLOOM_SAMPLES, BLOOM_SCALE);

    // Render final bloom result
    glBindFramebuffer(GL_FRAMEBUFFER, screenFBO);
    HDRBloomFinalShader.use();
    HDRBloomFinalShader.setFloat("exposure", exposure);
    HDRBloomFinalShader.setFloat("bloomStrength", BLOOM_STRENGTH);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrTex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, blurredTex);
    renderQuad();

    // FXAA

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    static Shader fxaaShader("FXAA.vs", "FXAA.fs");
    static Shader passShader("copy.vs", "copy.fs");
    if (fxaaEnabled)
      fxaaShader.use();
    else
      passShader.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, screenTex);
    renderQuad();

    if (debugSurface) {
      screenShader.use();
      screenShader.setInt("map", 0);
      screenShader.setInt("redOnly", 0);
      // depthDebugShader.use();
      // depthDebugShader.setInt("type", 0);
      // depthDebugShader.setFloat("near_plane", 0.1f);
      // depthDebugShader.setFloat("far_plane", LIGHT_FAR_PLANE);
      glDisable(GL_BLEND);
      glActiveTexture(GL_TEXTURE0);
      switch (debugSurface) {
      case 1:
        glBindTexture(GL_TEXTURE_2D, lightSystem.gPosition);
        break;
      case 2:
        screenShader.setInt("map", 1);
        glBindTexture(GL_TEXTURE_2D, lightSystem.gNormal);
        break;
      case 3:
        glBindTexture(GL_TEXTURE_2D, lightSystem.gAlbedo);
        break;
      case 4:
        glBindTexture(GL_TEXTURE_2D, lightSystem.gSpec);
        break;
      case 5:
        glBindTexture(GL_TEXTURE_2D, lightSystem.gLightAlbedo);
        break;
      case 6:
        glBindTexture(GL_TEXTURE_2D, lightSystem.gLightSpec);
        break;
      case 7:
        glBindTexture(GL_TEXTURE_2D, bloomTex);
        break;
      case 8:
        glBindTexture(GL_TEXTURE_2D, blurredTex);
        break;
      case 9:
        screenShader.setInt("redOnly", 1);
        glBindTexture(GL_TEXTURE_2D, lightSystem.ssaoMap);
        break;
      case 10:
        screenShader.setInt("redOnly", 1);
        glBindTexture(GL_TEXTURE_2D, lightSystem.ssaoMapBlurred);
        break;
      default:
        break;
      }
      renderQuad();
      glEnable(GL_BLEND);
    }

    glEnable(GL_BLEND);
    glBindVertexArray(0);
    glfwSwapBuffers(window);
    glfwPollEvents();
  }
}

int main() {
  // Init
  // ------------------
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_SAMPLES, MSAA_SAMPLES);
  // stbi_set_flip_vertically_on_load(true);

  GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT,
                                        "OpenGL Window Bruh", NULL, NULL);
  if (window == NULL) {
    std::cout << "Window booms" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  glEnable(GL_MULTISAMPLE);
  glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwSetScrollCallback(window, scroll_callback);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSetCursorPosCallback(window, mouse_callback);
  glfwSetKeyCallback(window, key_callback);

  // Enable Blending
  // --------------
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Shader advLightShader("light.vs", "light_v2.fs");

  // Go to the target scene
  // --------------
  Scene1(window);

  glfwTerminate();
  return 0;
}

bool firstMouse = true;
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
  if (firstMouse) // initially set to true
  {
    lastX = xpos;
    lastY = ypos;
    firstMouse = false;
  }

  float xoffset = xpos - lastX;
  float yoffset =
      lastY - ypos; // reversed since y-coordinates range from bottom to top
  lastX = xpos;
  lastY = ypos;

  mainCam.ProcessMouseMovement(xoffset, yoffset);
}