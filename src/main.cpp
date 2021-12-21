#include<iostream>
#include<vector>
#include<string>

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

static bool simulation_pause = false;

/** FUNCTIONS */
void update(const Window& window, double dt, Camera& camera);
GLuint gl_create_shader_program(const char* vs_code, const char* gs_code, const char* fs_code);

int main(void)
{
  // FHD: 1920, 1080, 2k: 2560, 1440
  Window window(1920, 1080); 
  glfwSwapInterval(0); // VSYNC

  /** ImGui setup begin */
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui_ImplGlfw_InitForOpenGL(window.get_native_window(), true);
  /** TODO: Remove hard coded glsl version */
  ImGui_ImplOpenGL3_Init("#version 330");
  /** ImGui setup end */

  GLuint skybox_shader_program = gl_create_shader_program(skybox_vs_code, nullptr, skybox_fs_code);
  GLuint particle_shader_program = gl_create_shader_program(particle_vs_code, particle_gs_code, particle_fs_code);
  float movement_speed = 10.0;
  float rotation_speed = 60.0;
  Camera camera(glm::vec3(-15.0, 12.0, -15.0),
    -135.f, 0.0f, 45.0f, 1260.0f / 1080.0f, 0.01, 1000.0, 
    rotation_speed, movement_speed
  );

  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Particle Shader Uniforms
  GLuint particle_view_loc = glGetUniformLocation(particle_shader_program, "u_ViewMatrix");
  GLuint particle_proj_loc = glGetUniformLocation(particle_shader_program, "u_ProjectionMatrix");
  GLuint cluster_count_loc = glGetUniformLocation(particle_shader_program, "u_ClusterCount");
  GLuint current_cluster_loc = glGetUniformLocation(particle_shader_program, "u_CurrentCluster");
  GLuint is_cluster_loc = glGetUniformLocation(particle_shader_program, "is_Cluster");
  GLuint bboxmin_loc = glGetUniformLocation(particle_shader_program, "u_BboxMin");
  GLuint bboxmax_loc = glGetUniformLocation(particle_shader_program, "u_BboxMax");
  GLuint particles_per_dim_loc = glGetUniformLocation(particle_shader_program, "u_ParticlesPerDim");

  int current_cluster = 0;

  // Setup Particle System
  glm::ivec3 particles_per_dim(7, 7, 7);
  glm::vec3 bbox_min(0, 0, 0);
  glm::vec3 bbox_max(5, 5, 5);
  ParticleSystem particle_system(particles_per_dim, bbox_min, bbox_max);

  // Skybox Shader Uniforms
  GLuint skybox_view_proj_loc = glGetUniformLocation(skybox_shader_program, "u_ViewProjection");
  GLuint skybox_texture_loc = glGetUniformLocation(skybox_shader_program, "cube_map");
  glUseProgram(skybox_shader_program);
  glUniform1i(skybox_texture_loc, 0);

  // Setup Skyboxes
  // Textures received from: https://www.humus.name/index.php?page=Textures
  const char* skyboxes_names[] = { "skansen", "ocean", "church"};
  static int current_skybox_idx = 0;
  const char* skybox_combo_label = skyboxes_names[current_skybox_idx];
  Skybox skyboxes[] = {Skybox(skyboxes_names[0], true), Skybox(skyboxes_names[1], true), Skybox(skyboxes_names[2], true)};
  glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

  bool draw_skybox = true;

  int n = 0;
  int fps_wrap = 200;
  std::vector<double> times(fps_wrap);
  std::vector<double> fps(fps_wrap);
  Clock clock;
  while (!window.should_close ()) {
    glClearColor(0.1, 0.11, 0.16, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

    double dt = simulation_pause ? 0 : clock.tick();
    update(window, dt, camera);
    double start = clock.since_start();
    times[n % fps_wrap] = (clock.since_start() - start) * 1000;
    fps[n % fps_wrap] = dt;
    n++;

    particle_system.update(dt);

    /** DRAW PARTICLES BEGIN **/
    glUseProgram(particle_shader_program);
    glUniformMatrix4fv(particle_view_loc, 1, false, &camera.get_view_matrix(true)[0][0]);
    glUniformMatrix4fv(particle_proj_loc, 1, false, &camera.get_projection_matrix()[0][0]);
    glUniform1f(cluster_count_loc, particle_system.get_num_clusters());
    glUniform1f(current_cluster_loc, current_cluster);
    glUniform1i(is_cluster_loc, 0);
    glUniform3f(bboxmin_loc, bbox_min.x, bbox_min.y, bbox_min.z);
    glUniform3f(bboxmax_loc, bbox_max.x, bbox_max.y, bbox_max.z);
    glUniform3i(particles_per_dim_loc, particles_per_dim.x, particles_per_dim.y, particles_per_dim.z);
    particle_system.draw_clusters();
    /** DRAW PARTICLES END **/

    /** SKYBOX RENDERING BEGIN (done last) **/
    if (draw_skybox) {
      glm::mat4 skybox_view_projection = camera.get_view_projection(false);
      glUseProgram(skybox_shader_program);
      glUniformMatrix4fv(skybox_view_proj_loc, 1, false, &skybox_view_projection[0][0]);
      skyboxes[current_skybox_idx].draw();
    }
    /** SKYBOX RENDERING END **/

    double update_sum = 0.0;
    double fps_sum = 0.0;
    for (int i = 0; i < fps_wrap; i++) {
      update_sum += times[i];
      fps_sum += fps[i];
    }
    update_sum /= fps_wrap;
    fps_sum /= fps_wrap;
      

    /** GUI RENDERING BEGIN **/
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("Settings panel");
    ImGui::Text("FPS: %f, time: %f (ms)", (1 / fps_sum), fps_sum * 1000);
    ImGui::Text("Update-time: %f (ms)", (update_sum));
    ImGui::Text("Simulation");
    ImGui::Dummy(ImVec2(0.0, 5.0));
    ImGui::SliderInt("Current cluster", &current_cluster, 0, particle_system.get_num_clusters());
    if (current_cluster < particle_system.get_num_clusters())
      ImGui::Text("Cluster count: %d", particle_system.get_cluster_count(current_cluster));

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

GLuint gl_create_shader_program(const char* vs_code, const char* gs_code, const char* fs_code) {
  int success;
  char infoLog[512];
  GLuint program = glCreateProgram();

  GLuint vs = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vs, 1, &vs_code, NULL);
  glCompileShader(vs);
  glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
  if(!success) {
    glGetShaderInfoLog(vs, sizeof(infoLog), NULL, infoLog);
    std::cout << "ERROR: Vertex shader compilation failed\n" << infoLog << std::endl;
  };
  glAttachShader(program, vs);

  GLuint gs = glCreateShader(GL_GEOMETRY_SHADER);
  if (gs_code != nullptr)
  {
    glShaderSource(gs, 1, &gs_code, NULL);
    glCompileShader(gs);
    glGetShaderiv(gs, GL_COMPILE_STATUS, &success);
    if (!success) {
      glGetShaderInfoLog(gs, sizeof(infoLog), NULL, infoLog);
      std::cout << "ERROR: Geometry shader compilation failed\n" << infoLog << std::endl;
    };
    glAttachShader(program, gs);
  }

  GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fs, 1, &fs_code, NULL);
  glCompileShader(fs);
  glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
  if(!success) {
    glGetShaderInfoLog(fs, sizeof(infoLog), NULL, infoLog);
    std::cout << "ERROR: Fragment shader compilation failed\n" << infoLog << std::endl;
  };
  glAttachShader(program, fs);

  glLinkProgram(program);
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if(!success) {
    glGetProgramInfoLog(program, sizeof(infoLog), NULL, infoLog);
    std::cout << "ERROR: Shader program linkage failed\n" << infoLog << std::endl;
  }  
  glDeleteShader(vs);
  glDeleteShader(gs);
  glDeleteShader(fs);
  return program;
};

