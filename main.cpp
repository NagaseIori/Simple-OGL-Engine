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
  auto vertices = boxVertices;
  unsigned int VBO, VAO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

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

  // Rendering Loop
  // ------------------
  while (!glfwWindowShouldClose(window)) {
    // Time Update
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // Pre-rendering
    process_input(window);

    // Rendering
    glClearColor(0.2f, 0.3f, 0.3f, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Fix Camera Y Position
    mainCam.Position.y = 1;

    // Draw Axis
    defaultShader.use();
    transformation(defaultShader);
    glBindVertexArray(AxisVAO);
    glDrawArrays(GL_LINES, 0, 6);

    // Draw my things
    float timeValue = glfwGetTime();
    float greenValue = (sin(timeValue) / 2.0f) + 0.5f;
    int vertexColorLoc = glGetUniformLocation(myShader.ID, "ourColor");

    myShader.use();
    myShader.setInt("texture1", 0);
    myShader.setInt("texture2", 1);
    myShader.setFloat("visibility", visibility);

    transformation(myShader);
    glUniform4f(vertexColorLoc, 0.0f, greenValue, .0, 1.0);
    glBindVertexArray(BoxVAO);
    for (unsigned int i = 0; i < 10; i++) {
      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, cubePositions[i]);
      float angle = 20.0f * i;
      model =
          glm::rotate(model, glm::radians(float(glfwGetTime() * 114.) + angle),
                      glm::vec3(1.0f, 0.3f, 0.5f));
      myShader.setMat4("model", model);

      glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    glBindVertexArray(0);

    // Post-rendering
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