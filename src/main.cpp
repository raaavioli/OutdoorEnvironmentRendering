#include<iostream>
#include<fstream>
#include<vector>
#include<string>
#include<map>

// External
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#define GL_SILENCE_DEPRECATION
// GL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Local
#include "texture.h"
#include "framebuffer.h"
#include "model.h"
#include "camera.h"
#include "clock.h"
#include "shader.h"
#include "window.h"
#include "particlesystem.h"
#include "gl_helpers.h"

static bool simulation_pause = false;

/** FUNCTIONS */
void update(const Window& window, double dt, Camera& camera);

int main(void)
{
  float movement_speed = 10.0;
  float rotation_speed = 60.0;
  Camera camera(glm::vec3(-15.0, 12.0, -15.0),
    -135.f, 0.0f, 45.0f, 1260.0f / 1080.0f, 0.01, 1000.0,
    rotation_speed, movement_speed
  );

  // FHD: 1920, 1080, 2k: 2560, 1440
  Window window(1920, 1080);
  glfwSwapInterval(0); // VSYNC

  /** ImGui setup begin */
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui_ImplGlfw_InitForOpenGL(window.get_native_window(), true);
  /** TODO: Remove hard coded glsl version */
  ImGui_ImplOpenGL3_Init("#version 430");
  /** ImGui setup end */

  GL_CHECK(glEnable(GL_CULL_FACE));
  GL_CHECK(glEnable(GL_DEPTH_TEST));
  GL_CHECK(glEnable(GL_BLEND));
  GL_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
  GL_CHECK(glPolygonMode( GL_FRONT_AND_BACK, GL_FILL));

  Shader skybox_shader("skybox.glsl");
  Shader particle_shader("particle.glsl");
  Shader particle_cs_shader("particle_cs.glsl");
  Shader quad_shader("textured_quad.glsl");

  // Setup Particle System
  glm::ivec3 particles_per_dim(160, 160, 160);
  glm::vec3 bbox_min(-250, -250, -250);
  glm::vec3 bbox_max(250, 250, 250);
  ParticleSystem particle_system(particles_per_dim, bbox_min, bbox_max);
  int current_cluster = particle_system.get_num_clusters();
  bool colored_particles = false;

  // Setup Skyboxes
  // Textures received from: https://www.humus.name/index.php?page=Textures
  const char* skyboxes_names[] = { "skansen", "ocean", "church"};
  static int current_skybox_idx = 0;
  const char* skybox_combo_label = skyboxes_names[current_skybox_idx];
  Skybox skyboxes[] = {Skybox(skyboxes_names[0], true), Skybox(skyboxes_names[1], true), Skybox(skyboxes_names[2], true)};
  GL_CHECK(glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS));

  Texture2D snow_tex = Texture2D("snowflake_non_commersial.png");

  FrameBuffer frame_buffer(1920, 1080);
  GLuint empty_vao;
  glGenVertexArrays(1, &empty_vao);

  bool draw_skybox = true;

  int n = 0;
  int fps_wrap = 200;
  std::vector<double> update_times(fps_wrap);
  std::vector<double> draw_times(fps_wrap);
  std::vector<double> fps(fps_wrap);
  Clock clock;
  GLenum err;
  double start = clock.since_start();;
  while (!window.should_close ()) {
    /** UPDATE BEGIN **/
    double dt = simulation_pause ? 0 : clock.tick();
    fps[n++ % fps_wrap] = dt;
    update(window, dt, camera);
    particle_system.update(dt, particle_cs_shader);
    /** UPDATE END **/

    /* DRAW SCENE ONTO FRAMEBUFFER */
    frame_buffer.bind();
    GL_CHECK(glClearColor(0.1, 0.11, 0.16, 1.0));
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    /** SKYBOX RENDERING BEGIN 
     * Usually done last, but particles are rendered after to enable transparent particles.
    **/
    if (draw_skybox) {
      glm::mat4 skybox_view_projection = camera.get_view_projection(false);
      skybox_shader.set_matrix4fv("u_ViewProjection", &skybox_view_projection[0][0]);
      skybox_shader.set_int("cube_map", 0);

      skybox_shader.bind();
      skyboxes[current_skybox_idx].bind_cube_map(0);
      skyboxes[current_skybox_idx].draw();
      skybox_shader.unbind();
    }
    /** SKYBOX RENDERING END **/

    /** DRAW PARTICLES BEGIN **/
    particle_shader.set_matrix4fv("u_ViewMatrix", &camera.get_view_matrix(true)[0][0]);
    particle_shader.set_matrix4fv("u_ProjectionMatrix", &camera.get_projection_matrix()[0][0]);
    particle_shader.set_int("u_ColoredParticles", (int)colored_particles);
    particle_shader.set_int("is_Cluster", 0);
    particle_shader.set_float("u_ClusterCount", particle_system.get_num_clusters());
    particle_shader.set_float("u_CurrentCluster", current_cluster);
    particle_shader.set_float3("u_BboxMin", bbox_min.x, bbox_min.y, bbox_min.z);
    particle_shader.set_float3("u_BboxMax", bbox_max.x, bbox_max.y, bbox_max.z);
    particle_shader.set_int3("u_ParticlesPerDim", particles_per_dim.x, particles_per_dim.y, particles_per_dim.z);
    particle_shader.set_int("u_particle_tex", 1);
    snow_tex.bind(1);

    particle_shader.bind();
    particle_system.draw();
    particle_shader.unbind();
    /** DRAW PARTICLES END **/
    frame_buffer.unbind();

    GL_CHECK(glClearColor(0.0, 1.0, 0.0, 1.0));
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
    /* DRAW FRAMEBUFFER ONTO SCREEN */
    GL_CHECK(glActiveTexture(GL_TEXTURE0));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, frame_buffer.get_color_attachment()));
    quad_shader.set_int("u_Texture", 0);
    GL_CHECK(glBindVertexArray(empty_vao));
    quad_shader.bind();
    GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, 6));
    GL_CHECK(glBindVertexArray(0));
    quad_shader.unbind();
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, 0));

    double update_sum = 0.0;
    double draw_sum = 0.0;
    double fps_sum = 0.0;
    for (int i = 0; i < fps_wrap; i++) {
      update_sum += update_times[i];
      draw_sum += draw_times[i];
      fps_sum += fps[i];
    }
    update_sum /= fps_wrap;
    draw_sum /= fps_wrap;
    fps_sum /= fps_wrap;

    /** GUI RENDERING BEGIN **/
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("Settings panel");
    ImGui::Text("FPS: %f, time: %f (ms)", (1 / fps_sum), fps_sum * 1000);
    ImGui::Text("Update-time: %f (ms)", update_sum * 1000);
    ImGui::Text("Draw-time: %f (ms)", draw_sum * 1000);
    
    ImGui::Dummy(ImVec2(0.0, 15.0));
    ImGui::Text("Simulation");
    ImGui::Text("Num particles: %d, (%d x %d x %d)", 
      particles_per_dim.x * particles_per_dim.y * particles_per_dim.z, 
      particles_per_dim.x, particles_per_dim.y, particles_per_dim.z);
    ImGui::Dummy(ImVec2(0.0, 5.0));
    ImGui::SliderInt("Current cluster", &current_cluster, 0, particle_system.get_num_clusters());
    ImGui::Checkbox("Colored particles", &colored_particles);

    ImGui::Dummy(ImVec2(0.0, 15.0));
    ImGui::Text("Environment");
    ImGui::Dummy(ImVec2(0.0, 5.0));
    ImGui::Checkbox("Enable skybox", &draw_skybox);
    
    if (ImGui::BeginCombo("Skybox", skybox_combo_label))
    {
      for (int n = 0; n < IM_ARRAYSIZE(skyboxes_names); n++)
      {
          const bool is_selected = (current_skybox_idx == n);
          if (ImGui::Selectable(skyboxes_names[n], is_selected)) current_skybox_idx = n;
          if (is_selected) ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }
    ImGui::Dummy(ImVec2(0.0, 15.0));

    ImGui::Text("Camera");
    ImGui::Dummy(ImVec2(0.0, 5.0));
    ImGui::SliderFloat("Movement speed", &movement_speed, 1.0, 10.0);
    ImGui::SliderFloat("Rotation speed", &rotation_speed, 1.0, 200.0);
    ImGui::Dummy(ImVec2(0.0, 15.0));
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    /** GUI RENDERING END **/

    camera.set_movement_speed(movement_speed);
    camera.set_rotation_speed(rotation_speed);

    window.resize(camera);
    window.swap_buffers ();
    window.poll_events ();
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  return 0;
}

void update(const Window& window, double dt, Camera& camera) {
  // std::cout << "Frame rate: " << 1.0 / dt << " FPS" << std::endl;

  if (window.is_key_pressed(GLFW_KEY_LEFT)) camera.rotate_yaw(dt);
  if (window.is_key_pressed(GLFW_KEY_RIGHT)) camera.rotate_yaw(-dt);
  if (window.is_key_pressed(GLFW_KEY_UP)) camera.rotate_pitch(dt);
  if (window.is_key_pressed(GLFW_KEY_DOWN)) camera.rotate_pitch(-dt);

  int direction = 0;
  if (window.is_key_pressed(GLFW_KEY_W)) direction |= Camera::FORWARD;
  if (window.is_key_pressed(GLFW_KEY_S)) direction |= Camera::BACKWARD;
  if (window.is_key_pressed(GLFW_KEY_D)) direction |= Camera::RIGHT;
  if (window.is_key_pressed(GLFW_KEY_A)) direction |= Camera::LEFT;
  camera.move(dt, direction);

  if (window.is_key_pressed(GLFW_KEY_P)) simulation_pause = !simulation_pause;
}

