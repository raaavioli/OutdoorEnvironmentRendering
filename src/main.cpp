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
#include "base_model.h"
#include "renderer.h"

/** SETTINGS VARIABLES **/
bool simulation_pause = false;
bool draw_shadow_map = false;
bool draw_colliders = true;
bool colored_particles = false;
bool depth_cull = false;
bool draw_quads = true;
bool draw_skybox_b = true;
bool draw_depthbuffer = false;

double update_sum = 0.0;
double draw_sum = 0.0;
double fps_sum = 0.0;

float u_WindAmp = 0.5f;
float u_WindFactor = 20.0f;
glm::vec2 u_WindDirection = glm::vec2(1.0, 1.0);

float quad_alpha = 1.0;
float ortho_size = 50.0f;
float ortho_far = 1000.0f;
float movement_speed = 15.0;
float rotation_speed = 100.0;

// Textures received from: https://www.humus.name/index.php?page=Textures
const char* skyboxes_names[] = { "skansen", "ocean", "church" };
static int current_skybox_idx = 1;
const char* skybox_combo_label = skyboxes_names[current_skybox_idx];

glm::ivec3 particles_per_dim(160, 160 * 2, 160);
glm::vec3 bbox_scale = glm::vec3(2, 1, 2) * 100.0f;
glm::vec3 bbox_center = glm::vec3(0, bbox_scale.y / 2 - 1, 0);
glm::vec3 bbox_min = bbox_center + glm::vec3(-0.5, -0.5, -0.5) * bbox_scale;
glm::vec3 bbox_max = bbox_center + glm::vec3(0.5, 0.5, 0.5) * bbox_scale;

glm::vec3 directional_light = glm::normalize(glm::vec3(-1.0, 1.0, -1.0));

enum MaterialIndex
{
  SHADED = 0,
  FLAT = 1
};

struct AABB
{
  glm::vec3 min;
  glm::vec3 max;
};

/** FUNCTIONS */
void update(const Window& window, double dt, Camera& camera);
void draw_models(std::vector<BaseModel> base_models, std::vector<BaseModel> quads, MaterialIndex index, const EnvironmentSettings& settings);
void draw_skybox(Shader& shader, Skybox& skybox, const EnvironmentSettings& settings);

void draw_to_screen(Shader& shader, GLuint texture);
void draw_gui();

int main(void)
{
  // FHD: 1920, 1080, 2k: 2560, 1440
  Window window(1920, 1080);

  Camera camera(glm::vec3(-15, 4, 12.5),
    -35.0, 0.0f, 45.0f, window.get_width() / (float) window.get_height(), 0.01, 1000.0,
    rotation_speed, movement_speed
  );

  glfwSwapInterval(0); // VSYNC

  /** ImGui setup begin */
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui_ImplGlfw_InitForOpenGL(window.get_native_window(), true);
  /** TODO: Remove hard coded glsl version */
  ImGui_ImplOpenGL3_Init("#version 460");
  /** ImGui setup end */

  GL_CHECK(glEnable(GL_CULL_FACE));
  GL_CHECK(glEnable(GL_DEPTH_TEST));
  GL_CHECK(glEnable(GL_BLEND));
  GL_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
  GL_CHECK(glPolygonMode( GL_FRONT_AND_BACK, GL_FILL));
  GL_CHECK(glEnable(GL_LINE_SMOOTH));

  Shader grass_shader =                 ShaderManager::Create("grass.glsl");
  Shader skybox_shader =                ShaderManager::Create("skybox.glsl");
  Shader particle_shader =              ShaderManager::Create("particle.glsl");
  Shader particle_cs_shader =           ShaderManager::Create("particle_cs.glsl");
  Shader particle_ms_shader =           ShaderManager::Create("particle_ms.glsl");
  Shader framebuffer_shader =           ShaderManager::Create("framebuffer.glsl");
  Shader raw_model_shader =             ShaderManager::Create("raw_model.glsl");
  Shader raw_model_flat_color_shader =  ShaderManager::Create("raw_model_flat_color.glsl");
  int work_group_sizes[3];
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_group_sizes[0]);
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_group_sizes[1]);
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_group_sizes[2]);
  std::cout << "GL_MAX_COMPUTE_WORK_GROUP_SIZE: " << work_group_sizes[0] << ", " << work_group_sizes[1] << ", " << work_group_sizes[2] << std::endl;


  // Setup Particle System
  Texture2D snow_tex("snowflake_non_commersial.png");
  ParticleSystem particle_system(particles_per_dim, bbox_min, bbox_max);
  int current_cluster = particle_system.get_num_clusters();

  Texture2D noise_marble_tex("noisemarble1.png");

  glm::ivec2 grass_per_dim(5000, 5000);
  ParticleSystem grass_system(glm::ivec3(grass_per_dim.x, 1, grass_per_dim.y), glm::vec3(bbox_min.x, 0, bbox_min.z), glm::vec3(bbox_max.x, 0, bbox_max.z));

  // Setup Skyboxes
  Skybox skyboxes[] = {Skybox(skyboxes_names[0], true), Skybox(skyboxes_names[1], true), Skybox(skyboxes_names[2], true)};
  GL_CHECK(glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS));

  FrameBuffer frame_buffer(window.get_width(), window.get_height());
  // TODO: Don't create color attachment when only depth attachment is needed
  FrameBuffer shadow_map_buffer(2048, 2048);

  glm::vec4 vertex_color(1.0, 1.0, 1.0, 1.0);
  std::vector<Vertex> quad_vertices{
    {glm::vec3(-0.5, -0.5, 0.0), vertex_color, glm::vec3(0.0, 0.0, 1.0), glm::vec2(0.0, 1.0)},
    {glm::vec3(0.5, -0.5, 0.0), vertex_color, glm::vec3(0.0, 0.0, 1.0), glm::vec2(1.0, 1.0)},
    {glm::vec3(0.5, 0.5, 0.0), vertex_color, glm::vec3(0.0, 0.0, 1.0), glm::vec2(1.0, 0.0)},
    {glm::vec3(-0.5, 0.5, 0.0), vertex_color, glm::vec3(0.0, 0.0, 1.0), glm::vec2(0.0, 0.0)},
  };
  std::vector<uint32_t> quad_indices{
    0, 1, 2,
    2, 3, 0,
  };

  std::vector<Vertex> cube_vertices{
    {glm::vec3(-0.5, -0.5, 0.5), vertex_color, glm::vec3(0.0, 0.0, 1.0), glm::vec2(0.0, 1.0)}, // Front-face
    {glm::vec3(0.5, -0.5, 0.5), vertex_color, glm::vec3(0.0, 0.0, 1.0), glm::vec2(1.0, 1.0)},
    {glm::vec3(0.5, 0.5, 0.5), vertex_color, glm::vec3(0.0, 0.0, 1.0), glm::vec2(1.0, 0.0)},
    {glm::vec3(-0.5, 0.5, 0.5), vertex_color, glm::vec3(0.0, 0.0, 1.0), glm::vec2(0.0, 0.0)},

    {glm::vec3(0.5, -0.5, -0.5), vertex_color, glm::vec3(1.0, 0.0, 0.0), glm::vec2(1.0, 1.0)}, // Right-face
    {glm::vec3(0.5, 0.5, -0.5), vertex_color, glm::vec3(1.0, 0.0, 0.0), glm::vec2(1.0, 0.0)},
    {glm::vec3(0.5, 0.5, 0.5), vertex_color, glm::vec3(1.0, 0.0, 0.0), glm::vec2(0.0, 0.0)},
    {glm::vec3(0.5, -0.5, 0.5), vertex_color, glm::vec3(1.0, 0.0, 0.0), glm::vec2(0.0, 1.0)},

    {glm::vec3(-0.5, -0.5, -0.5), vertex_color, glm::vec3(0.0, 0.0, -1.0), glm::vec2(1.0, 1.0)}, // Back-face
    {glm::vec3(-0.5, 0.5, -0.5), vertex_color, glm::vec3(0.0, 0.0, -1.0), glm::vec2(1.0, 0.0)},
    {glm::vec3(0.5, 0.5, -0.5), vertex_color, glm::vec3(0.0, 0.0, -1.0), glm::vec2(0.0, 0.0)},
    {glm::vec3(0.5, -0.5, -0.5), vertex_color, glm::vec3(0.0, 0.0, -1.0), glm::vec2(0.0, 1.0)},

    {glm::vec3(-0.5, -0.5, -0.5), vertex_color, glm::vec3(-1.0, 0.0, 0.0), glm::vec2(0.0, 1.0)}, // Left-face
    {glm::vec3(-0.5, -0.5, 0.5), vertex_color, glm::vec3(-1.0, 0.0, 0.0), glm::vec2(1.0, 1.0)},
    {glm::vec3(-0.5, 0.5, 0.5), vertex_color, glm::vec3(-1.0, 0.0, 0.0), glm::vec2(1.0, 0.0)},
    {glm::vec3(-0.5, 0.5, -0.5), vertex_color, glm::vec3(-1.0, 0.0, 0.0), glm::vec2(0.0, 0.0)},

    {glm::vec3(-0.5, -0.5, -0.5), vertex_color, glm::vec3(0.0, -1.0, 0.0), glm::vec2(0.0, 1.0)}, // Bottom-face
    {glm::vec3(0.5, -0.5, -0.5), vertex_color, glm::vec3(0.0, -1.0, 0.0), glm::vec2(1.0, 1.0)},
    {glm::vec3(0.5, -0.5, 0.5), vertex_color, glm::vec3(0.0, -1.0, 0.0), glm::vec2(1.0, 0.0)},
    {glm::vec3(-0.5, -0.5, 0.5), vertex_color, glm::vec3(0.0, -1.0, 0.0), glm::vec2(0.0, 0.0)},

    {glm::vec3(-0.5, 0.5, -0.5), vertex_color, glm::vec3(0.0, 1.0, 0.0), glm::vec2(0.0, 1.0)}, // Up-face
    {glm::vec3(-0.5, 0.5, 0.5), vertex_color, glm::vec3(0.0, 1.0, 0.0), glm::vec2(1.0, 1.0)},
    {glm::vec3(0.5, 0.5, 0.5), vertex_color, glm::vec3(0.0, 1.0, 0.0), glm::vec2(1.0, 0.0)},
    {glm::vec3(0.5, 0.5, -0.5), vertex_color, glm::vec3(0.0, 1.0, 0.0), glm::vec2(0.0, 0.0)},
  };

  std::vector<uint32_t> cube_indices;
  cube_indices.reserve(6 * 6);
  for (int i = 0; i < 6; i++)
  {
    int idx = i * 4;
    cube_indices.push_back(idx + 0);
    cube_indices.push_back(idx + 1);
    cube_indices.push_back(idx + 2);
    cube_indices.push_back(idx + 2);
    cube_indices.push_back(idx + 3);
    cube_indices.push_back(idx + 0);
  }

  Texture2D white_tex;
  RawModel quad_raw(quad_vertices, quad_indices, GL_STATIC_DRAW);
  RawModelMaterial shaded_quad_mat(&raw_model_shader, glm::vec4(101, 67, 33, 255.0f) / 255.0f, white_tex.get_texture_id(), shadow_map_buffer.get_depth_attachment());
  RawModelFlatColorMaterial flat_quad_mat(&raw_model_flat_color_shader, glm::vec4(0.0, 1.0, 1.0, 1.0));
  BaseModel quad_model;
  quad_model.raw_model = &quad_raw;
  quad_model.materials[MaterialIndex::SHADED] = &shaded_quad_mat;
  quad_model.materials[MaterialIndex::FLAT] = &flat_quad_mat;

  RawModel cube_raw(cube_vertices, cube_indices, GL_STATIC_DRAW);
  RawModelMaterial shaded_cube_mat(&raw_model_shader, glm::vec4(1.0, 1.0, 1.0, 1.0), white_tex.get_texture_id(), shadow_map_buffer.get_depth_attachment());
  RawModelFlatColorMaterial flat_cube_mat(&raw_model_flat_color_shader, glm::vec4(0.0, 1.0, 0.0, 1.0));
  BaseModel cube_model;
  cube_model.raw_model = &cube_raw;
  cube_model.materials[MaterialIndex::SHADED] = &shaded_cube_mat;
  cube_model.materials[MaterialIndex::FLAT] = &flat_cube_mat;

  // RawModel workbench_model("workbench.fbx");

  Texture2D color_palette_tex("color_palette.png");
  RawModel garage_raw("garage.fbx");
  RawModelMaterial shaded_garage_mat(&raw_model_shader, glm::vec4(1.0, 1.0, 1.0, 1.0), color_palette_tex.get_texture_id(), shadow_map_buffer.get_depth_attachment());
  RawModelFlatColorMaterial flat_garage_mat(&raw_model_flat_color_shader, glm::vec4(1.0, 0.0, 1.0, 1.0));
  BaseModel garage_model;
  garage_model.raw_model = &garage_raw;
  garage_model.materials[MaterialIndex::SHADED] = &shaded_garage_mat;
  garage_model.materials[MaterialIndex::FLAT] = &flat_garage_mat;


  Texture2D wood_workbench_tex("carpenterbench_albedo.png");
  RawModel wood_workbench_raw("wood_workbench.fbx");
  RawModelMaterial shaded_workbench_mat(&raw_model_shader, glm::vec4(1.0, 1.0, 1.0, 1.0), wood_workbench_tex.get_texture_id(), shadow_map_buffer.get_depth_attachment());
  RawModelFlatColorMaterial flat_workbench_mat(&raw_model_flat_color_shader, glm::vec4(1.0, 1.0, 0.0, 1.0));
  BaseModel wood_workbench_model;
  wood_workbench_model.raw_model = &wood_workbench_raw;
  wood_workbench_model.materials[MaterialIndex::SHADED] = &shaded_workbench_mat;
  wood_workbench_model.materials[MaterialIndex::FLAT] = &flat_workbench_mat;
  wood_workbench_model.transform = glm::translate(glm::vec3(-8.0, 1.1, 0.0)) * glm::rotate(glm::quarter_pi<float>(), glm::vec3(0, 1, 0)) * glm::mat4(1.0);

  Texture2D container_tex("container_albedo.png");
  RawModel container_raw("container.fbx");
  RawModelMaterial shaded_container_mat(&raw_model_shader, glm::vec4(1.0, 1.0, 1.0, 1.0), container_tex.get_texture_id(), shadow_map_buffer.get_depth_attachment());
  RawModelFlatColorMaterial flat_container_mat(&raw_model_flat_color_shader, glm::vec4(0.0, 0.0, 1.0, 1.0));
  BaseModel container_model;
  container_model.raw_model = &container_raw;
  container_model.materials[MaterialIndex::SHADED] = &shaded_container_mat;
  container_model.materials[MaterialIndex::FLAT] = &flat_container_mat;
  container_model.transform = glm::rotate(glm::half_pi<float>(), glm::vec3(0, 1, 0)) * glm::scale(glm::vec3(1, 1, 1)) * glm::mat4(1.0);

  RawModel bunny_raw("stanford-bunny.fbx");
  RawModelMaterial shaded_bunny_mat(&raw_model_shader, glm::vec4(1.0, 1.0, 1.0, 1.0), white_tex.get_texture_id(), shadow_map_buffer.get_depth_attachment());
  RawModelFlatColorMaterial flat_bunny_mat(&raw_model_flat_color_shader, glm::vec4(0.0, 0.0, 1.0, 1.0));
  BaseModel bunny_model;
  bunny_model.raw_model = &bunny_raw;
  bunny_model.materials[MaterialIndex::SHADED] = &shaded_bunny_mat;
  bunny_model.materials[MaterialIndex::FLAT] = &flat_bunny_mat;
  bunny_model.transform = glm::translate(glm::vec3(-8.0, 20.0, 0.0)) * glm::rotate(glm::quarter_pi<float>() / 2.0f, glm::vec3(1, 0, 0)) * glm::mat4(1.0);

  std::vector<glm::vec3> garage_positions = { 
    glm::vec3(15.0, 0.0, 0.0), 
    glm::vec3(15.0, 0.0, 22.5), 
    glm::vec3(15.0, 0.0, 45.0), 
    glm::vec3(15.0, 0.0, 60.0)
  };
  std::vector<glm::vec3> garage_sizes = {
    glm::vec3(1.0),
    glm::vec3(2.0),
    glm::vec3(1.0),
    glm::vec3(1.0)
  };
  std::vector<AABB> colliders;
  for (int i = 0; i < garage_positions.size(); i++)
    colliders.push_back({ garage_positions[i] - glm::vec3(6.0f, 0.0f, 5.0f) * garage_sizes[i], garage_positions[i] + glm::vec3(5.0f, 5.5f, 5.0f) * garage_sizes[i] });

  int n = 0;
  int fps_wrap = 200;
  std::vector<double> update_times(fps_wrap);
  std::vector<double> draw_times(fps_wrap);
  std::vector<double> fps(fps_wrap);
  Clock clock;
  GLenum err;
  double start = clock.since_start();

  EnvironmentSettings environment_settings;

  static float time = 0.0f;

  while (!window.should_close ()) {
    /** UPDATE BEGIN **/
    double dt = simulation_pause ? 0 : clock.tick();
    time += dt;
    fps[n++ % fps_wrap] = dt;
    update(window, dt, camera);
    particle_system.update(dt, particle_cs_shader);

    std::vector<BaseModel> quads;
    quad_model.transform = glm::rotate(-glm::half_pi<float>(), glm::vec3(1.0, 0.0, 0.0)) * glm::scale(glm::vec3(500, 500, 1)) * glm::mat4(1.0);
    quads.push_back(quad_model);
    quad_model.transform = glm::translate(glm::vec3(0.0, 2.0, -8.0)) * glm::scale(glm::vec3(25, 3, 1)) * glm::mat4(1.0);
    quads.push_back(quad_model);

    std::vector<BaseModel> models;
    models.push_back(container_model);
    models.push_back(wood_workbench_model);
    //models.push_back(bunny_model);
    for (int i = 0; i < garage_positions.size(); i++)
    {
      garage_model.transform = glm::translate(garage_positions[i]) * glm::rotate(-glm::half_pi<float>(), glm::vec3(0, 1, 0)) * glm::scale(garage_sizes[i]) * glm::mat4(1.0);
      models.push_back(garage_model);
    }

    glm::mat4 light_view = glm::lookAt(directional_light * ortho_size, glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
    glm::mat4 light_view_projection = glm::ortho<float>(-ortho_size, ortho_size, -ortho_size, ortho_size, 0.1, ortho_far) * light_view;
    /** UPDATE END **/

    /* DRAW SCENE TO SHADOW MAP */
    shadow_map_buffer.bind();
    GL_CHECK(glClearColor(0.0, 0.0, 0.0, 1.0));
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
    GL_CHECK(glEnable(GL_DEPTH_TEST));
    {
      environment_settings.camera_view_projection = light_view_projection;
      draw_models(models, quads, MaterialIndex::FLAT, environment_settings);
    }
    shadow_map_buffer.unbind();

    /* DRAW SCENE TO BACKBUFFER */
    frame_buffer.bind();
    GL_CHECK(glClearColor(0.1, 0.11, 0.16, 1.0));
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
    GL_CHECK(glEnable(GL_DEPTH_TEST));
    /** SKYBOX RENDERING BEGIN, drawn early to enable transparent objects **/
    if (draw_skybox_b) {
      environment_settings.camera_view_projection = camera.get_view_projection(false);
      draw_skybox(skybox_shader, skyboxes[current_skybox_idx], environment_settings);
    }
    /** SKYBOX RENDERING END **/

    environment_settings.camera_view_projection = camera.get_view_projection(true);
    environment_settings.directional_light = directional_light;
    environment_settings.light_view_projection = light_view_projection;
    draw_models(models, quads, MaterialIndex::SHADED, environment_settings);

    if (draw_colliders)
    {
      for (auto& collider : colliders)
      {
        GL_CHECK(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));
        GL_CHECK(glDisable(GL_CULL_FACE));
        cube_model.material_index = MaterialIndex::FLAT;
        cube_model.transform = glm::translate((collider.max + collider.min) / 2.0f) * glm::scale(collider.max - collider.min) * glm::mat4(1.0);
        Renderer::draw(cube_model, environment_settings);
        GL_CHECK(glEnable(GL_CULL_FACE));
        GL_CHECK(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
      }
    }

    grass_shader.bind();
    grass_shader.set_matrix4fv("u_ViewMatrix", &camera.get_view_matrix(true)[0][0]);
    grass_shader.set_matrix4fv("u_ProjectionMatrix", &camera.get_projection_matrix()[0][0]);
    grass_shader.set_float3("u_SystemBoundsMin", bbox_min.x, 0, bbox_min.z);
    grass_shader.set_float3("u_SystemBoundsMax", bbox_max.x, 0, bbox_max.z);
    grass_shader.set_int3("u_ParticlesPerDim", grass_per_dim.x, 1, grass_per_dim.y);
    grass_shader.set_float("u_Time", time);
    grass_shader.set_float("u_WindAmp", u_WindAmp);
    grass_shader.set_float("u_WindFactor", u_WindFactor);
    grass_shader.set_float2("u_WindDirection", u_WindDirection.x, u_WindDirection.y);
    grass_shader.set_int("u_WindTexture", 0);
    noise_marble_tex.bind(0);
    grass_system.draw_instanced(4);
    grass_shader.unbind();

    /** DRAW PARTICLES BEGIN 
    {
        particle_shader.bind();
        particle_shader.set_matrix4fv("u_ViewMatrix", &camera.get_view_matrix(true)[0][0]);
        particle_shader.set_matrix4fv("u_ProjectionMatrix", &camera.get_projection_matrix()[0][0]);
        particle_shader.set_int("u_DepthCull", (int)depth_cull);
        particle_shader.set_int("u_ColoredParticles", (int)colored_particles);
        particle_shader.set_int("is_Cluster", 0);
        particle_shader.set_float("u_ClusterCount", particle_system.get_num_clusters());
        particle_shader.set_float("u_CurrentCluster", current_cluster);
        particle_shader.set_float3("u_SystemBoundsMin", bbox_min.x, bbox_min.y, bbox_min.z);
        particle_shader.set_float3("u_SystemBoundsMax", bbox_max.x, bbox_max.y, bbox_max.z);
        particle_shader.set_int3("u_ParticlesPerDim", particles_per_dim.x, particles_per_dim.y, particles_per_dim.z);
        particle_shader.set_int("u_DepthTexture", 0);
        for (int i = 0; i < colliders.size(); i++)
        {
          AABB& collider = colliders[i];
          std::string uniform_start = "u_CullingBoxes[" + std::to_string(i);
          std::string uniform_min = uniform_start + "].min";
          std::string uniform_max = uniform_start + "].max";
          particle_shader.set_float3(uniform_min, collider.min.x, collider.min.y, collider.min.z);
          particle_shader.set_float3(uniform_max, collider.max.x, collider.max.y, collider.max.z);
        }
        GL_CHECK(glActiveTexture(GL_TEXTURE0));
        GL_CHECK(glBindTexture(GL_TEXTURE_2D, frame_buffer.get_depth_attachment()));
        snow_tex.bind(1);
        particle_shader.set_int("u_particle_tex", 1);

        particle_system.draw();
        particle_shader.unbind();
    }
    /** DRAW PARTICLES END **/

    /* RENDER TO DEFAULT FRAMEBUFFER + PRESENT */
    {
        frame_buffer.unbind();
        FrameBuffer& rendered_buffer = draw_shadow_map ? shadow_map_buffer : frame_buffer;
        GLuint rendered_texture = draw_depthbuffer ? rendered_buffer.get_depth_attachment() : rendered_buffer.get_color_attachment();
        GL_CHECK(glClearColor(0.0, 0.0, 1.0, 1.0));
        GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
        GL_CHECK(glDisable(GL_DEPTH_TEST));
        draw_to_screen(framebuffer_shader, rendered_texture);
        //particle_ms_shader.bind();
        //particle_ms_shader.set_float("u_Scale", 0.5f);
        //GL_CHECK(glDrawMeshTasksNV(0, 1));
    }

    update_sum = 0.0;
    draw_sum = 0.0;
    fps_sum = 0.0;
    for (int i = 0; i < fps_wrap; i++) {
      update_sum += update_times[i];
      draw_sum += draw_times[i];
      fps_sum += fps[i];
    }
    update_sum /= fps_wrap;
    draw_sum /= fps_wrap;
    fps_sum /= fps_wrap;

    /** GUI RENDERING BEGIN **/
    draw_gui();
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

void draw_models(std::vector<BaseModel> base_models, std::vector<BaseModel> quads, MaterialIndex index, const EnvironmentSettings& settings)
{
  if (draw_quads)
  {
    GL_CHECK(glDepthMask(depth_cull || quad_alpha >= 1.0f ? GL_TRUE : GL_FALSE));
    GL_CHECK(glDisable(GL_CULL_FACE));
    for (BaseModel& model : quads)
    {
      model.material_index = index;
      Renderer::draw(model, settings);
    }
    GL_CHECK(glEnable(GL_CULL_FACE));
    GL_CHECK(glDepthMask(GL_TRUE));
  }

  for (BaseModel& model : base_models)
  {
    model.material_index = index;
    Renderer::draw(model, settings);
  }
}

void draw_skybox(Shader& shader, Skybox& skybox, const EnvironmentSettings& settings)
{
  shader.bind();
  shader.set_matrix4fv("u_ViewProjection", &settings.camera_view_projection[0][0]);
  shader.set_int("cube_map", 0);

  skybox.bind_cube_map(0);
  skybox.draw();
  shader.unbind();
}

void draw_to_screen(Shader& shader, GLuint texture)
{
  static GLuint empty_vao = 0;
  if (!empty_vao)
    GL_CHECK(glGenVertexArrays(1, &empty_vao));
  GL_CHECK(glBindVertexArray(empty_vao));
  shader.bind();
  shader.set_int("u_Texture", 0);
  GL_CHECK(glActiveTexture(GL_TEXTURE0));
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, texture));
  shader.set_int("u_DrawDepth", (int)draw_depthbuffer);
  GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, 6));
  GL_CHECK(glBindVertexArray(0));
  shader.unbind();
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, 0));
}

void draw_gui()
{
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  ImGui::Begin("Settings panel");
  ImGui::Text("FPS: %f, time: %f (ms)", (1 / fps_sum), fps_sum * 1000);
  ImGui::Text("Update-time: %f (ms)", update_sum * 1000);
  ImGui::Text("Draw-time: %f (ms)", draw_sum * 1000);

  ImGui::Dummy(ImVec2(0.0, 15.0));
  ImGui::Text("Shaders");
  if (ImGui::Button("Reload"))
    ShaderManager::Reload();


  ImGui::Dummy(ImVec2(0.0, 15.0));
  ImGui::Text("Simulation");
  ImGui::Text("Num particles: %d, (%d x %d x %d)",
    particles_per_dim.x * particles_per_dim.y * particles_per_dim.z,
    particles_per_dim.x, particles_per_dim.y, particles_per_dim.z);
  ImGui::Dummy(ImVec2(0.0, 5.0));
  ImGui::Checkbox("Colored particles", &colored_particles);

  ImGui::Dummy(ImVec2(0.0, 15.0));
  ImGui::Text("Environment");
  ImGui::Dummy(ImVec2(0.0, 5.0));
  ImGui::Checkbox("Enable skybox", &draw_skybox_b);
  ImGui::Checkbox("Draw depthbuffer", &draw_depthbuffer);
  ImGui::Checkbox("Depth cull", &depth_cull);
  ImGui::Checkbox("Draw quad", &draw_quads);
  if (draw_quads)
    ImGui::SliderFloat("Quad alpha", &quad_alpha, 0.0, 1.0);
  ImGui::Checkbox("Draw colliders", &draw_colliders);

  ImGui::Text("Wind");
  ImGui::Dummy(ImVec2(0.0, 5.0));
  ImGui::SliderFloat2("Wind direction", (float*) &u_WindDirection, -1.0, 1.0);
  ImGui::SliderFloat("WindAmp", &u_WindAmp, 0.0, 1.0);
  ImGui::SliderFloat("WindFactor", &u_WindFactor, 0.0, 40.0);

  ImGui::Dummy(ImVec2(0.0, 15.0));
  ImGui::Text("Lighting");
  ImGui::Dummy(ImVec2(0.0, 5.0));
  ImGui::SliderFloat3("Directional Light", &directional_light[0], -1, 1);
  directional_light = glm::normalize(directional_light);
  ImGui::SliderFloat("Ortho size", &ortho_size, 1, 1000);
  ImGui::SliderFloat("Ortho far", &ortho_far, 1, 1000);
  ImGui::Checkbox("Draw shadow map", &draw_shadow_map);

  ImGui::Dummy(ImVec2(0.0, 15.0));
  if (ImGui::BeginCombo("Skybox", skybox_combo_label))
  {
    for (int n = 0; n < IM_ARRAYSIZE(skyboxes_names); n++)
    {
      const bool is_selected = (current_skybox_idx == n);
      if (ImGui::Selectable(skyboxes_names[n], is_selected))
      {
        current_skybox_idx = n;
        skybox_combo_label = skyboxes_names[current_skybox_idx];
      }
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
}