#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "shader_s.h"
#include "model.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "camera.h"
#include "utils.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdio>
#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080
#define FAR_PLANE 1000000.f

float visibility = 0.2;
float camera_height = 3.0;
float deltaTime = 0.0f; // Time between current frame and last frame
float lastFrame = 0.0f; // Time of last frame
float lastX = WINDOW_WIDTH / 2., lastY = WINDOW_HEIGHT / 2.;
float spotLightEnabled = 1.0;
bool depthDebug = false;

Camera mainCam(30.f, 30.f, 3.f, 0.f, 1.f, 0.f, -135.f, -45.f);

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
}

void scroll_callback(GLFWwindow *window, double xoff, double yoff) {
  mainCam.ProcessMouseScroll(yoff);
}
void mouse_callback(GLFWwindow *window, double xpos, double ypos);

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

unsigned int load_texture(const std::string image, unsigned int type) {
  unsigned int texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  // set the texture wrapping/filtering options (on the currently bound texture
  // object)
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  // load and generate the texture
  int width, height, nrChannels;
  unsigned char *data =
      stbi_load(image.c_str(), &width, &height, &nrChannels, 0);
  if (data) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, type,
                 GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
  } else {
    std::cout << "Failed to load texture" << std::endl;
  }
  stbi_image_free(data);
  return texture;
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

unsigned int getBoxVAO() {
  auto &vertices = boxVertices;
  // VBO & VAO Setting up
  unsigned int VBO, VAO, EBO;
  glGenBuffers(1, &EBO);
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(2);

  // Texture loading
  unsigned int texture1 = load_texture("container.jpg", GL_RGB);
  unsigned int texture2 = load_texture("awesomeface.png", GL_RGBA);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture1);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, texture2);
  glEnable(GL_DEPTH_TEST);
  glBindVertexArray(0);
  return VAO;
}

auto getLightVAO() {
  float vertices[] = {
      -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, 0.0f,  0.0f,  0.5f,  -0.5f,
      -0.5f, 0.0f,  0.0f,  -1.0f, 1.0f,  0.0f,  0.5f,  0.5f,  -0.5f, 0.0f,
      0.0f,  -1.0f, 1.0f,  1.0f,  0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f,
      1.0f,  1.0f,  -0.5f, 0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f, 0.0f,  1.0f,
      -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, 0.0f,  0.0f,

      -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.5f,  -0.5f,
      0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,  0.5f,  0.5f,  0.5f,  0.0f,
      0.0f,  1.0f,  1.0f,  1.0f,  0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
      1.0f,  1.0f,  -0.5f, 0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
      -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

      -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f,  1.0f,  0.0f,  -0.5f, 0.5f,
      -0.5f, -1.0f, 0.0f,  0.0f,  1.0f,  1.0f,  -0.5f, -0.5f, -0.5f, -1.0f,
      0.0f,  0.0f,  0.0f,  1.0f,  -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f,
      0.0f,  1.0f,  -0.5f, -0.5f, 0.5f,  -1.0f, 0.0f,  0.0f,  0.0f,  0.0f,
      -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f,  1.0f,  0.0f,

      0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.5f,  0.5f,
      -0.5f, 1.0f,  0.0f,  0.0f,  1.0f,  1.0f,  0.5f,  -0.5f, -0.5f, 1.0f,
      0.0f,  0.0f,  0.0f,  1.0f,  0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,  0.0f,
      0.0f,  1.0f,  0.5f,  -0.5f, 0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
      0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

      -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,  0.0f,  1.0f,  0.5f,  -0.5f,
      -0.5f, 0.0f,  -1.0f, 0.0f,  1.0f,  1.0f,  0.5f,  -0.5f, 0.5f,  0.0f,
      -1.0f, 0.0f,  1.0f,  0.0f,  0.5f,  -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,
      1.0f,  0.0f,  -0.5f, -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,  0.0f,  0.0f,
      -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,  0.0f,  1.0f,

      -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.5f,  0.5f,
      -0.5f, 0.0f,  1.0f,  0.0f,  1.0f,  1.0f,  0.5f,  0.5f,  0.5f,  0.0f,
      1.0f,  0.0f,  1.0f,  0.0f,  0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
      1.0f,  0.0f,  -0.5f, 0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
      -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,  0.0f,  1.0f};
  unsigned int VBO, VAO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                        (void *)(6 * sizeof(float)));
  glEnableVertexAttribArray(2);

  unsigned int diffuseMap = load_texture("container2.png", GL_RGBA);
  unsigned int specularMap =
      load_texture("lighting_maps_specular_color.png", GL_RGBA);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, diffuseMap);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, specularMap);

  return VAO;
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

void drawAxis(Shader &shader, unsigned int AxisVAO) {
  shader.use();
  transformation(shader);
  glBindVertexArray(AxisVAO);
  glDrawArrays(GL_LINES, 0, 6);
}

void setupScreenFBO(const unsigned int FBO, unsigned int &texture_o) {
  glBindFramebuffer(GL_FRAMEBUFFER, FBO);

  unsigned int texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGB,
               GL_UNSIGNED_BYTE, NULL);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         texture, 0);

  unsigned int rbo;
  glGenRenderbuffers(1, &rbo);

  glBindRenderbuffer(GL_RENDERBUFFER, rbo);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, WINDOW_WIDTH,
                        WINDOW_HEIGHT);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                            GL_RENDERBUFFER, rbo);

  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                            GL_RENDERBUFFER, rbo);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!"
              << std::endl;
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  texture_o = texture;
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
  for (int i = 0; i < 20; i++) {
    cubePositions.push_back(
        {rand() % 100 - 50, rand() % 100 - 50, rand() % 100 - 50});
  }
  for (int i = 0; i < 18; i++)
    pointLightPositions.push_back(
        {rand() % 100 - 50, rand() % 100 - 50, rand() % 100 - 50});

  // Get needed VAOs
  // ------------------
  unsigned int AxisVAO = getAxisVAO();
  unsigned int BoxVAO = getBoxVAO();

  // Light Settings
  // ------------------
  Shader lightSourceShader("light.vs", "light_source.fs");
  Shader directionLightShader("light.vs", "light_direction.fs");
  Shader pointLightShader("light.vs", "light_point.fs");
  Shader spotLightShader("light.vs", "light_spot.fs");
  Shader lightShader("light.vs", "light_v2.fs");
  unsigned int lightCubeVAO = getLightVAO();

  // Load Models
  // ------------------
  Model modelBag("model/backpack/backpack.obj");
  // Model modelGirl("model/girl/girl.usdc");
  Model modelSponza("model/sponza/sponza.obj");

  // Setup Screen Framebuffer & Shader
  unsigned int screenFBO, quadVAO, screenTex;
  glGenFramebuffers(1, &screenFBO);
  setupScreenFBO(screenFBO, screenTex);
  Shader screenShader("screen.vs", "screen.fs");
  quadVAO = getQuadVAO();

  // Rendering Loop
  // ------------------
  glm::mat4 model(1.0f);

  // Depthmap setup
  // -----------------
  unsigned int depthMapFBO;
  glGenFramebuffers(1, &depthMapFBO);
  const unsigned int SHADOW_WIDTH = 8192, SHADOW_HEIGHT = 8192;
  unsigned int depthMap;
  glGenTextures(1, &depthMap);
  glBindTexture(GL_TEXTURE_2D, depthMap);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH,
               SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
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

  glm::vec3 dirLightDirection(-0.5, -1.0f, -0.5);
  Shader simpleDepthShader("simpleDepthShader.vs", "simpleDepthShader.fs");
  Shader depthDebugShader("depthShaderDebug.vs", "depthShaderDebug.fs");

  auto renderScene = [&](Shader &shader) {
    shader.use();
    // Draw boxes
    // -------------
    for (unsigned int i = 0; i < cubePositions.size(); i++) {
      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(
          model, cubePositions[i] +
                     glm::vec3(0.f, sin(glfwGetTime() + i * 1.14) * 4.f, 0.f));
      float angle = 20.0f * i;
      model = glm::rotate(model, float(glfwGetTime() + i * 0.5),
                          glm::vec3(1.0f, 0.3f, 0.5f));
      // model = glm::scale(model, glm::vec3(4.f));
      shader.setMat4("model", model);

      modelBag.Draw(shader);
      // modelMiku.Draw(shader);
    }

    // Draw Sponza
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.f, -50.f, 0.f));
    model = glm::scale(model, glm::vec3(0.1f));
    shader.setMat4("model", model);
    modelSponza.Draw(shader);

    // Draw Girl
    // lightShader.setMat4("model",
    //                     glm::scale(glm::mat4(1.f), glm::vec3(0.05f)));
    // modelGirl.Draw(lightShader);
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
    drawAxis(defaultShader, AxisVAO);

    // Fix Camera Y Position
    // -----------------
    // mainCam.Position.y = 1;

    // Draw my things
    // -----------------
    glm::vec3 lightColor(1.f);

    glm::vec3 diffuseColor = lightColor * glm::vec3(0.5f);
    glm::vec3 ambientColor = diffuseColor * glm::vec3(0.2f);
    glm::vec3 specularColor = lightColor * glm::vec3(1.f);

    lightShader.use();
    transformation(lightShader);
    glBindVertexArray(lightCubeVAO);
    // Setup Basics
    lightShader.setVec3("viewPos", mainCam.Position);
    lightShader.setFloat("material.shininess", 2.0f);

    // Setup Lights
    // -------------
    lightShader.setInt("lightCount", pointLightPositions.size() + 2);
    /// Spot Light
    // -----------------
    lightShader.setVec3("lights[0].position", mainCam.Position);
    lightShader.setVec3("lights[0].direction", mainCam.Front);
    lightShader.setFloat("lights[0].cutOff", glm::cos(glm::radians(30.f)));
    lightShader.setFloat("lights[0].outerCutOff", glm::cos(glm::radians(35.f)));
    lightShader.setVec3("lights[0].ambient",
                        ambientColor * 2.f * spotLightEnabled);
    lightShader.setVec3("lights[0].diffuse",
                        diffuseColor * 2.f * spotLightEnabled);
    lightShader.setVec3("lights[0].specular",
                        specularColor * 2.f * spotLightEnabled);
    lightShader.setFloat("lights[0].constant", 1.0f);
    lightShader.setFloat("lights[0].linear", 0.045f);
    lightShader.setFloat("lights[0].quadratic", 0.0075f);
    lightShader.setInt("lights[0].type", 2);
    /// Direction Light
    // -----------------
    // lightColor = glm::vec3(1.f, 0.3f, 0.f);  // sunset
    lightColor = RGBColor(80,104,134);  // moonlight

    diffuseColor = lightColor * 0.4f;
    ambientColor = diffuseColor * 0.1f;
    specularColor = lightColor * 0.f;
    lightShader.setVec3("lights[1].direction", dirLightDirection);
    lightShader.setVec3("lights[1].ambient", ambientColor);
    lightShader.setVec3("lights[1].diffuse", diffuseColor);
    lightShader.setVec3("lights[1].specular", specularColor);
    lightShader.setInt("lights[1].type", 1);

    /// Point Lights
    // -----------------
    for (int i = 0; i < pointLightPositions.size(); i++) {
      glm::vec3 lightPosOffset;
      lightPosOffset.x = cos(glfwGetTime() + i * 11.4) * 12.0;
      lightPosOffset.y = sin(glfwGetTime() * 3 + i * 11.4) * 12.0;
      lightPosOffset.z = sin(glfwGetTime() + i * 11.4) * 12.0;

      glm::vec3 lightColor(1.f);
      float time = glfwGetTime();
      lightColor.x = sin(time + i * 11.4) / 2 + 0.5;
      lightColor.y = cos(time + i * 11.4) / 2 + 0.5;
      lightColor.z = sin(time - i * 11.4) / 2 + 0.5;
      lightColor *= 2.f;

      glm::vec3 diffuseColor = lightColor * glm::vec3(0.5f);
      glm::vec3 ambientColor = diffuseColor * glm::vec3(0.2f);
      glm::vec3 specularColor = lightColor * glm::vec3(1.f);

      glm::vec3 pos = pointLightPositions[i] + lightPosOffset;
      lightShader.use();
      std::string light_prefix =
          std::string("lights[") + std::to_string(i + 2) + "].";
      auto elestr = [&](std::string element) { return light_prefix + element; };
      lightShader.setVec3(elestr("position"), pos);
      lightShader.setVec3(elestr("ambient"), ambientColor);
      lightShader.setVec3(elestr("diffuse"), diffuseColor);
      lightShader.setVec3(elestr("specular"), specularColor);
      lightShader.setFloat(elestr("constant"), 1.0f);
      lightShader.setFloat(elestr("linear"), 0.09f);
      lightShader.setFloat(elestr("quadratic"), 0.032f);
      lightShader.setInt(elestr("type"), 0);

      lightSourceShader.use();
      glBindVertexArray(lightCubeVAO);
      transformation(lightSourceShader);
      model = glm::mat4(1.0f);
      model = glm::translate(model, pos);
      model = glm::scale(model, glm::vec3(0.2f));
      lightSourceShader.setMat4("model", model);
      lightSourceShader.setVec3("lightColor", lightColor);
      glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    // Render to depthmap
#define LIGHT_INF 300.f
#define DEPTH_FAR_PLANE 1200.f
    glm::mat4 lightProjection = glm::ortho(-LIGHT_INF, LIGHT_INF, -LIGHT_INF,
                                           LIGHT_INF, 0.1f, DEPTH_FAR_PLANE);
    glm::vec3 lightPosition = glm::vec3(150.f, 300.f, 150.f);
    glm::mat4 lightView =
        glm::lookAt(lightPosition, lightPosition + dirLightDirection,
                    glm::vec3(0.f, 1.f, 0.f));
    glm::mat4 lightSpaceMatrix = lightProjection * lightView;
    simpleDepthShader.use();
    simpleDepthShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
    simpleDepthShader.setVec3("lightDir", dirLightDirection);

    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    // glCullFace(GL_FRONT);
    // glDisable(GL_CULL_FACE);
    renderScene(simpleDepthShader);
    // glEnable(GL_CULL_FACE);
    // glCullFace(GL_BACK);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, screenFBO);
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    lightShader.use();
    lightShader.setInt("lights[1].shadowCast", 1);
    glActiveTexture(GL_TEXTURE20);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    lightShader.setInt("lights[1].shadowMap", 20);
    lightShader.setMat4("lights[1].lightSpace", lightSpaceMatrix);

    renderScene(lightShader);
    // Post-rendering
    // -----------------
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // back to default
    glClearColor(.0f, 1.0f, .0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    screenShader.use();
    glBindVertexArray(quadVAO);
    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);
    screenShader.setInt("screenTexture", 0);
    glBindTexture(GL_TEXTURE_2D, screenTex);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    if (depthDebug) {
      depthDebugShader.use();
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, depthMap);
      glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    glBindVertexArray(0);
    glfwSwapBuffers(window);
    glfwPollEvents();
  }
}

int main() {
  // Init
  // ------------------
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_SAMPLES, 4);
  stbi_set_flip_vertically_on_load(true);

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
  glEnable(GL_FRAMEBUFFER_SRGB); // Gamma Correction for now

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