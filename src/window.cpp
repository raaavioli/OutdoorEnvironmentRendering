#include "window.h"

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

#include <iostream>
#include <map>

Window::Window(uint32_t width, uint32_t height) : width(width), height(height) {
  MTL::Device* pDevice = MTL::CreateSystemDefaultDevice();

  if (!glfwInit()) {
    std::cout << "Error: Could not initialize glfw" << std::endl;
    exit(EXIT_FAILURE);
  }

#ifdef __APPLE__
  /* We need to explicitly ask for a 3.2 context on OS X */
  glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#elif
  glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4); // We want OpenGL 4.6
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
#endif 

  this->window = glfwCreateWindow(width, height, "Rendering environment", NULL, NULL);
  if (!this->window) {
    glfwTerminate();
    std::cout << "ERROR: Could not create glfw window" << std::endl;
    exit(EXIT_FAILURE);
  }
  glfwMakeContextCurrent(this->window);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    /* Problem: glewInit failed, something is seriously wrong. */
    std::cerr << "ERROR: failed to initialize OpenGL context \n" << std::endl;
  }

  std::cout << "Using GL version: " << glGetString(GL_VERSION) << std::endl;
  std::cout << "Shading language version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

  /* Query extensions */
  std::vector<std::string> required_extensions = {"GL_NV_mesh_shader", "GL_ARB_compute_shader"};
  list_extension_availability(required_extensions);
}

void Window::list_extension_availability(std::vector<std::string> required_extensions)
{
  int n_extensions;
  glGetIntegerv(GL_NUM_EXTENSIONS, &n_extensions);
  std::cout << "Available extensions: " << n_extensions << std::endl;

  std::map<std::string, std::string> extensions_supported;
  for (auto& s : required_extensions)
    extensions_supported[s] = "MISSING";

  for (int i = 0; i < n_extensions; i++)
  {
    const char* extension = (const char*)glGetStringi(GL_EXTENSIONS, i);
    for (auto& req_ext : required_extensions)
    {
      if (!req_ext.compare(extension)) {
        extensions_supported[req_ext] = "FOUND";
      }
    }
  }

  std::cout << "Required extensions: " << std::endl;
  for (auto& [extension, status] : extensions_supported)
    std::cout << "\t" << extension << ": " << status << std::endl;
}