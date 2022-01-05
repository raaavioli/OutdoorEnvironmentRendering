#pragma once

#include <string>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "camera.h"

struct Window {
public:
  Window(uint32_t width, uint32_t height);

  ~Window() {
    glfwTerminate();
  }

  inline bool should_close() { return glfwWindowShouldClose(this->window); }
  inline void poll_events() { glfwPollEvents(); }
  inline void swap_buffers() { glfwSwapBuffers(this->window); }
  inline GLFWwindow* get_native_window() { return this->window; }

  void resize(Camera& camera) {
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    camera.set_aspect(display_w / (float)display_h);
    glViewport(0, 0, display_w, display_h);
  }

  bool is_key_pressed(int keycode) const {
    return glfwGetKey(window, keycode) == GLFW_PRESS;
  }

private:
  void list_extension_availability(std::vector<std::string>);

private:
  GLFWwindow* window;
};
