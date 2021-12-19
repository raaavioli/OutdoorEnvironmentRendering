#include "window.h"

#include <iostream>

Window::Window(uint32_t width, uint32_t height) {
  if (!glfwInit()) {
    std::cout << "Error: Could not initialize glfw" << std::endl;
    exit(EXIT_FAILURE);
  }

  glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL 

  this->window = glfwCreateWindow(width, height, "Clustered particles", NULL, NULL);
  if (!this->window) {
    glfwTerminate();
    std::cout << "Could not create glfw window" << std::endl;
    exit(EXIT_FAILURE);
  }
  glfwMakeContextCurrent(this->window);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    /* Problem: glewInit failed, something is seriously wrong. */
    std::cerr << "Error: failed to initialize OpenGL context \n" << std::endl;
  }

  std::cout << "Using GL version: " << glGetString(GL_VERSION) << std::endl;
  std::cout << "Shading language version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
}