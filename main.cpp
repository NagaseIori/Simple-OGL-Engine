#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "shader_s.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "camera.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdio>
#include <iostream>
#include <cmath>
#include <fstream>

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080
#define FAR_PLANE 1000.f

float visibility = 0.2;
float camera_height = 3.0;
float deltaTime = 0.0f; // Time between current frame and last frame
float lastFrame = 0.0f; // Time of last frame
float lastX = WINDOW_WIDTH / 2., lastY = WINDOW_HEIGHT / 2.;
float spotLightEnabled = 1.0;

Camera mainCam(3.f, 3.f, 3.f, 0.f, 1.f, 0.f, -135.f, -45.f);

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
  const float cameraSpeed = 7.5f * deltaTime;
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    mainCam.ProcessKeyboard(Camera_Movement::FORWARD, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    mainCam.ProcessKeyboard(Camera_Movement::BACKWARD, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    mainCam.ProcessKeyboard(Camera_Movement::LEFT, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    mainCam.ProcessKeyboard(Camera_Movement::RIGHT, deltaTime);
  if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    spotLightEnabled = 1.0;
  else {
    spotLightEnabled = 0.0;
  }
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

int main() {
  // Init
  // ------------------
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
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

  glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwSetScrollCallback(window, scroll_callback);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSetCursorPosCallback(window, mouse_callback);

  // Shaders compilation
  // ------------------
  Shader myShader("learn.vs", "learn.fs");
  Shader defaultShader("default.vs", "default.fs");

  glm::vec3 cubePositions[] = {
      glm::vec3(0.0f, 0.0f, 0.0f),    glm::vec3(2.0f, 5.0f, -15.0f),
      glm::vec3(-1.5f, -2.2f, -2.5f), glm::vec3(-3.8f, -2.0f, -12.3f),
      glm::vec3(2.4f, -0.4f, -3.5f),  glm::vec3(-1.7f, 3.0f, -7.5f),
      glm::vec3(1.3f, -2.0f, -2.5f),  glm::vec3(1.5f, 2.0f, -2.5f),
      glm::vec3(1.5f, 0.2f, -1.5f),   glm::vec3(-1.3f, 1.0f, -1.5f)};

  // Get needed VAOs
  // ------------------
  unsigned int AxisVAO = getAxisVAO();
  unsigned int BoxVAO = getBoxVAO();

  // Light Settings
  // ------------------
  glm::vec3 lightPos(1.2f, 1.0f, 2.0f);
  Shader lightSourceShader("light.vs", "light_source.fs");
  Shader directionLightShader("light.vs", "light_direction.fs");
  Shader pointLightShader("light.vs", "light_point.fs");
  Shader spotLightShader("light.vs", "light_spot.fs");
  Shader lightShader("light.vs", "light.fs");
  unsigned int lightCubeVAO = getLightVAO();

  // Rendering Loop
  // ------------------
  glm::mat4 model(1.0f);
  while (!glfwWindowShouldClose(window)) {
    // Time Update
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // Something else
    lightPos.x = cos(glfwGetTime()) * 2.0;
    lightPos.y = sin(glfwGetTime() * 3) * 2.0;
    lightPos.z = sin(glfwGetTime()) * 2.0;

    glm::vec3 lightColor(1.f);

    glm::vec3 diffuseColor = lightColor * glm::vec3(0.5f);
    glm::vec3 ambientColor = diffuseColor * glm::vec3(0.2f);
    glm::vec3 specularColor = lightColor * glm::vec3(1.f);

    // Pre-rendering
    process_input(window);

    // Rendering
    glClearColor(0.2f, 0.3f, 0.3f, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Fix Camera Y Position
    // mainCam.Position.y = 1;

    // Draw Axis
    defaultShader.use();
    transformation(defaultShader);
    glBindVertexArray(AxisVAO);
    glDrawArrays(GL_LINES, 0, 6);

    // Draw my things
    lightShader.use();
    transformation(lightShader);
    glBindVertexArray(lightCubeVAO);
    // Setup Material
    lightShader.setVec3("objectColor", glm::vec3(1.0f, 0.5f, 0.31f));
    lightShader.setVec3("viewPos", mainCam.Position);
    lightShader.setInt("material.diffuse", 0);
    lightShader.setInt("material.specular", 1);
    lightShader.setFloat("material.shininess", 32.0f);

    // Setup Lights
    // -------------
    lightShader.setInt("lightCount", 7);
    /// Spot Light
    lightShader.setVec3("lights[0].position", mainCam.Position);
    lightShader.setVec3("lights[0].direction", mainCam.Front);
    lightShader.setFloat("lights[0].cutOff", glm::cos(glm::radians(12.5f)));
    lightShader.setFloat("lights[0].outerCutOff", glm::cos(glm::radians(15.f)));
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
    lightShader.setVec3("lights[1].direction", -0.2f, -1.0f, -0.3f);
    lightShader.setVec3("lights[1].ambient", ambientColor * 0.3f);
    lightShader.setVec3("lights[1].diffuse", diffuseColor * 0.3f);
    lightShader.setVec3("lights[1].specular", specularColor * 0.3f);
    lightShader.setInt("lights[1].type", 1);
    /// Point Lights
    glm::vec3 pointLightPositions[] = {
        glm::vec3(0.7f, 0.2f, 2.0f), glm::vec3(2.3f, -3.3f, -4.0f),
        glm::vec3(-4.0f, 2.0f, -12.0f), glm::vec3(0.0f, 0.0f, -3.0f), lightPos};

    for (int i = 0; i < 5; i++) {
      glm::vec3 lightColor(1.f);
      float time = glfwGetTime();
      lightColor.x = sin(time + i * 11.4) / 2 + 0.5;
      lightColor.y = cos(time + i * 11.4) / 2 + 0.5;
      lightColor.z = sin(time - i * 11.4) / 2 + 0.5;
      lightColor += glm::vec3(0.33f);

      glm::vec3 diffuseColor = lightColor * glm::vec3(0.5f);
      glm::vec3 ambientColor = diffuseColor * glm::vec3(0.2f);
      glm::vec3 specularColor = lightColor * glm::vec3(1.f);

      lightShader.use();
      std::string light_prefix =
          std::string("lights[") + std::to_string(i + 2) + "].";
      auto elestr = [&](std::string element) { return light_prefix + element; };
      lightShader.setVec3(elestr("position"), pointLightPositions[i]);
      lightShader.setVec3(elestr("ambient"), ambientColor);
      lightShader.setVec3(elestr("diffuse"), diffuseColor);
      lightShader.setVec3(elestr("specular"), specularColor);
      lightShader.setFloat(elestr("constant"), 1.0f);
      lightShader.setFloat(elestr("linear"), 0.045f);
      lightShader.setFloat(elestr("quadratic"), 0.0075f);
      lightShader.setInt(elestr("type"), 0);

      lightSourceShader.use();
      glBindVertexArray(lightCubeVAO);
      transformation(lightSourceShader);
      model = glm::mat4(1.0f);
      model = glm::translate(model, pointLightPositions[i]);
      model = glm::scale(model, glm::vec3(0.2f));
      lightSourceShader.setMat4("model", model);
      lightSourceShader.setVec3("lightColor", lightColor);
      glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    lightShader.use();
    // Draw boxes
    // -------------
    for (unsigned int i = 0; i < 10; i++) {
      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, cubePositions[i]);
      float angle = 20.0f * i;
      model = glm::rotate(model, float(glfwGetTime() + i * 0.5),
                          glm::vec3(1.0f, 0.3f, 0.5f));
      lightShader.setMat4("model", model);

      glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    // Post-rendering
    glBindVertexArray(0);
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

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