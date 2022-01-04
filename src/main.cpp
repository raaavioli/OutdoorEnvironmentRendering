#include<iostream>
#include<fstream>
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
#include "gl_helpers.h"

static bool simulation_pause = false;

/** FUNCTIONS */
void update(const Window& window, double dt, Camera& camera);
GLuint gl_create_shader_program(const char* vs_code, const char* gs_code, const char* fs_code);
GLuint gl_create_compute_program(const char* cs_code);

std::string read_file(const std::string& file_path);

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
  GL_CHECK(glClearColor(0.1, 0.11, 0.16, 1.0));
  GL_CHECK(glPolygonMode( GL_FRONT_AND_BACK, GL_FILL));

  const char* skybox_vs_code;
  const char* skybox_fs_code;
  const char* particle_vs_code;
  const char* particle_gs_code;
  const char* particle_fs_code;
  const char* particle_cs_code;

  std::string assets_base = "assets/";
  std::string shader_base = assets_base + "shaders/";
  std::string file_name = assets_base + "particle.glsl";

  /* PARSE GLSL SHADER */
  std::string skybox_source = read_file((shader_base + std::string("skybox.glsl")));
  std::string particle_source = read_file((shader_base + std::string("particle.glsl")));
  std::string particle_cs_source = read_file((shader_base + std::string("particle_cs.glsl")));

  GLuint skybox_shader_program = gl_create_shader_program(skybox_vs_code, nullptr, skybox_fs_code);
  GLuint particle_shader_program = gl_create_shader_program(particle_vs_code, particle_gs_code, particle_fs_code);
  GLuint particle_simulation_program = gl_create_compute_program(particle_cs_code);

  // Particle Shader Uniforms
  GLuint particle_view_loc = glGetUniformLocation(particle_shader_program, "u_ViewMatrix");
  GLuint particle_proj_loc = glGetUniformLocation(particle_shader_program, "u_ProjectionMatrix");
  GLuint cluster_count_loc = glGetUniformLocation(particle_shader_program, "u_ClusterCount");
  GLuint current_cluster_loc = glGetUniformLocation(particle_shader_program, "u_CurrentCluster");
  GLuint is_cluster_loc = glGetUniformLocation(particle_shader_program, "is_Cluster");
  GLuint bboxmin_loc = glGetUniformLocation(particle_shader_program, "u_BboxMin");
  GLuint bboxmax_loc = glGetUniformLocation(particle_shader_program, "u_BboxMax");
  GLuint u_colored_particles = glGetUniformLocation(particle_shader_program, "u_ColoredParticles");
  GLuint particles_per_dim_loc = glGetUniformLocation(particle_shader_program, "u_ParticlesPerDim");

  Texture2D snow_tex = Texture2D("snowflake_non_commersial.png");
  GLuint u_particle_tex = glGetUniformLocation(skybox_shader_program, "u_particle_tex");
  GL_CHECK(glUseProgram(particle_simulation_program));
  GL_CHECK(glUniform1i(u_particle_tex, 1));
  GL_CHECK(glUseProgram(0));

  // Setup Particle System
  glm::ivec3 particles_per_dim(160, 160, 160);
  glm::vec3 bbox_min(-250, -250, -250);
  glm::vec3 bbox_max(250, 250, 250);
  ParticleSystem particle_system(particles_per_dim, bbox_min, bbox_max, particle_simulation_program);
  int current_cluster = particle_system.get_num_clusters();
  bool colored_particles = false;

  // Skybox Shader Uniforms
  GLuint skybox_view_proj_loc = glGetUniformLocation(skybox_shader_program, "u_ViewProjection");
  GLuint skybox_texture_loc = glGetUniformLocation(skybox_shader_program, "cube_map");
  GL_CHECK(glUseProgram(skybox_shader_program));
  GL_CHECK(glUniform1i(skybox_texture_loc, 0));

  // Setup Skyboxes
  // Textures received from: https://www.humus.name/index.php?page=Textures
  const char* skyboxes_names[] = { "skansen", "ocean", "church"};
  static int current_skybox_idx = 0;
  const char* skybox_combo_label = skyboxes_names[current_skybox_idx];
  Skybox skyboxes[] = {Skybox(skyboxes_names[0], true), Skybox(skyboxes_names[1], true), Skybox(skyboxes_names[2], true)};
  GL_CHECK(glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS));

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
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    double dt = simulation_pause ? 0 : clock.tick();
    update(window, dt, camera);
    fps[n % fps_wrap] = dt;
    n++;

    particle_system.update(dt);

    /** SKYBOX RENDERING BEGIN 
     * Usually done last, but particles are rendered after to enable transparent particles.
    **/
    if (draw_skybox) {
      glm::mat4 skybox_view_projection = camera.get_view_projection(false);
      GL_CHECK(glUseProgram(skybox_shader_program));
      GL_CHECK(glUniformMatrix4fv(skybox_view_proj_loc, 1, false, &skybox_view_projection[0][0]));
      skyboxes[current_skybox_idx].draw();
    }
    /** SKYBOX RENDERING END **/

    /** DRAW PARTICLES BEGIN **/
    GL_CHECK(glUseProgram(particle_shader_program));
    GL_CHECK(glUniformMatrix4fv(particle_view_loc, 1, false, &camera.get_view_matrix(true)[0][0]));
    GL_CHECK(glUniformMatrix4fv(particle_proj_loc, 1, false, &camera.get_projection_matrix()[0][0]));
    GL_CHECK(glUniform1i(u_colored_particles, (int)colored_particles));
    GL_CHECK(glUniform1f(cluster_count_loc, particle_system.get_num_clusters()));
    GL_CHECK(glUniform1f(current_cluster_loc, current_cluster));
    GL_CHECK(glUniform1i(is_cluster_loc, 0));
    GL_CHECK(glUniform3f(bboxmin_loc, bbox_min.x, bbox_min.y, bbox_min.z));
    GL_CHECK(glUniform3f(bboxmax_loc, bbox_max.x, bbox_max.y, bbox_max.z));
    GL_CHECK(glUniform3i(particles_per_dim_loc, particles_per_dim.x, particles_per_dim.y, particles_per_dim.z));
    snow_tex.bind(1);
    particle_system.draw();
    /** DRAW PARTICLES END **/

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

GLuint gl_create_shader_program(const char* vs_code, const char* gs_code, const char* fs_code) {
  int success;
  char info_log[512];
  GLuint program = glCreateProgram();

  GLuint vs = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vs, 1, &vs_code, NULL);
  glCompileShader(vs);
  glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
  if(!success) {
    glGetShaderInfoLog(vs, sizeof(info_log), NULL, info_log);
    std::cout << "ERROR: Vertex shader compilation failed\n" << info_log << std::endl;
  };
  glAttachShader(program, vs);

  GLuint gs = glCreateShader(GL_GEOMETRY_SHADER);
  if (gs_code != nullptr)
  {
    glShaderSource(gs, 1, &gs_code, NULL);
    glCompileShader(gs);
    glGetShaderiv(gs, GL_COMPILE_STATUS, &success);
    if (!success) {
      glGetShaderInfoLog(gs, sizeof(info_log), NULL, info_log);
      std::cout << "ERROR: Geometry shader compilation failed\n" << info_log << std::endl;
    };
    glAttachShader(program, gs);
  }

  GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fs, 1, &fs_code, NULL);
  glCompileShader(fs);
  glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
  if(!success) {
    glGetShaderInfoLog(fs, sizeof(info_log), NULL, info_log);
    std::cout << "ERROR: Fragment shader compilation failed\n" << info_log << std::endl;
  };
  glAttachShader(program, fs);

  glLinkProgram(program);
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if(!success) {
    glGetProgramInfoLog(program, sizeof(info_log), NULL, info_log);
    std::cout << "ERROR: Shader program linkage failed\n" << info_log << std::endl;
  }  
  glDeleteShader(vs);
  glDeleteShader(gs);
  glDeleteShader(fs);
  return program;
}

GLuint gl_create_compute_program(const char* cs_code)
{
  GLuint cs = glCreateShader(GL_COMPUTE_SHADER);
  glShaderSource(cs, 1, &cs_code, NULL);
  glCompileShader(cs);

  int success;
  char info_log[512];
  glGetShaderiv(cs, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(cs, sizeof(info_log), NULL, info_log);
    std::cout << "ERROR: Compute shader compilation failed\n" << info_log << std::endl;
  };

  GLuint program = glCreateProgram();
  glAttachShader(program, cs);
  glLinkProgram(program);
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(program, sizeof(info_log), NULL, info_log);
    std::cout << "ERROR: Compute shader program linkage failed\n" << info_log << std::endl;
  }

  glDeleteShader(cs);
  return program;
}

std::string read_file(const std::string& file_path)
{
  /* READ FILE */
  std::string result;
  std::ifstream in(file_path, std::ios::in | std::ios::binary);
  if (in)
  {
    in.seekg(0, std::ios::end);
    size_t size = in.tellg();
    if (size != -1)
    {
      result.resize(size);
      in.seekg(0, std::ios::beg);
      in.read(&result[0], size);
    }
    else
    {
      std::cout << "Could not read file: " << file_path << std::endl;
    }
  }
  else
  {
    std::cout << "Could not open file: " << file_path << std::endl;
  }
  return result;
}

