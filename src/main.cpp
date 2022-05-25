#include<iostream>
#include<fstream>
#include<vector>
#include<string>
#include<map>

// External
#include "imgui.h"
#include "ImGuizmo.h"
#include "backends/imgui_impl_glfw.h"
#ifdef __APPLE__
#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define MTK_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include <AppKit/AppKit.hpp>
#include "backends/imgui_impl_metal.h"
#else
#include "backends/imgui_impl_opengl3.h"
#endif

// GL
#define GL_SILENCE_DEPRECATION
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
#include "renderer.h"
#include "input.h"
#include "entity.h"
#include "components.h"

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

// Wind
float u_WindAmp = 0.3f;
float u_WindFactor = 20.0f;
glm::vec2 u_WindDirection = glm::vec2(1.0, 1.0);

// Grass
bool g_DrawGrass = true;
int32_t vertices_per_blade = 8;

float quad_alpha = 1.0;
float ortho_size = 25.0f;
float ortho_far = 100.0f;
float movement_speed = 15.0;
float rotation_speed = 100.0;

Entity g_selected_entity = Entity::Invalid();

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
void draw_models(MaterialIndex index, const EnvironmentSettings& settings);
void draw_skybox(Shader& shader, Skybox& skybox, const EnvironmentSettings& settings);

void draw_to_screen(Shader& shader, GLuint texture);
void draw_gui(const Camera& camera);

int main(void)
{
    // FHD: 1920, 1080, 2k: 2560, 1440
    Window window(1920, 1080);
    Input::Init(window.get_native_window());

    Camera camera(glm::vec3(-15, 4, 12.5),
        -35.0, 0.0f, 45.0f, window.get_width() / (float)window.get_height(), 0.01, 1000.0,
        rotation_speed, movement_speed
    );

    glfwSwapInterval(0); // VSYNC

    /** ImGui setup begin */
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();


#ifdef __APPLE__
    ImGui_ImplMetal_Init(pDevice);
    ImGui_ImplGlfw_InitForOther(window.get_native_window(), true);
#else
    ImGui_ImplOpenGL3_Init("#version 460");
    ImGui_ImplGlfw_InitForOpenGL(window.get_native_window(), true);
#endif
    /** ImGui setup end */

    GL_CHECK(glEnable(GL_CULL_FACE));
    GL_CHECK(glEnable(GL_DEPTH_TEST));
    GL_CHECK(glDisable(GL_BLEND));
    //GL_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    // TODO: Draw transparent objects later in the pipeline, such as particles
    GL_CHECK(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
    GL_CHECK(glEnable(GL_LINE_SMOOTH));

    Shader* grass_shader = ShaderManager::Create("grass.glsl");
    Shader* skybox_shader = ShaderManager::Create("skybox.glsl");
    Shader* particle_shader = ShaderManager::Create("particle.glsl");
    Shader* particle_cs_shader = ShaderManager::Create("particle_cs.glsl");
    //Shader particle_ms_shader =           ShaderManager::Create("particle_ms.glsl");
    Shader* framebuffer_shader = ShaderManager::Create("framebuffer.glsl");
    Shader* raw_model_shader = ShaderManager::Create("raw_model.glsl");
    Shader* raw_model_flat_color_shader = ShaderManager::Create("raw_model_flat_color.glsl");
    Shader* variance_shadow_map_shader = ShaderManager::Create("variance_shadow_map.glsl");

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

    glm::ivec2 grass_per_dim(3000, 3000);
    ParticleSystem grass_system(glm::ivec3(grass_per_dim.x, 1, grass_per_dim.y), glm::vec3(bbox_min.x, 0, bbox_min.z) / 32.0f, glm::vec3(bbox_max.x, 0, bbox_max.z) / 32.0f);

    // Setup Skyboxes
    Skybox skyboxes[] = { Skybox(skyboxes_names[0], true), Skybox(skyboxes_names[1], true), Skybox(skyboxes_names[2], true) };
    GL_CHECK(glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS));

    FrameBufferCreateInfo fb_cinfo;
    {
        fb_cinfo.width = window.get_width();
        fb_cinfo.height = window.get_height();
        fb_cinfo.attachment_bits = AttachmentType::COLOR | AttachmentType::DEPTH;
        fb_cinfo.num_color_attachments = 2;

        FrameBufferTextureCreateInfo texture_cinfos[2];
        texture_cinfos[0].color_format = ColorFormat::RGBA8;
        texture_cinfos[1].color_format = ColorFormat::R32UI;
        texture_cinfos[1].min_filter = GL_NEAREST;
        texture_cinfos[1].mag_filter = GL_NEAREST;
        fb_cinfo.color_attachment_infos = texture_cinfos;
    }
    FrameBuffer frame_buffer(fb_cinfo);

    FrameBufferCreateInfo shadow_cinfo;
    shadow_cinfo.width = shadow_cinfo.height = 4096;
    shadow_cinfo.attachment_bits = AttachmentType::DEPTH;
    shadow_cinfo.num_color_attachments = 0;
    FrameBuffer shadow_map_buffer(shadow_cinfo);

    FrameBufferCreateInfo vsm_cinfo;
    vsm_cinfo.width = vsm_cinfo.height = 4096;
    vsm_cinfo.attachment_bits = AttachmentType::COLOR;
    vsm_cinfo.num_color_attachments = 1;
    FrameBufferTextureCreateInfo vsm_tex_cinfo = {  ColorFormat::RG16, GL_LINEAR, GL_LINEAR };
    vsm_cinfo.color_attachment_infos = &vsm_tex_cinfo;
    FrameBuffer variance_map_buffer(vsm_cinfo, true);

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
    RawModelMaterial shaded_quad_mat(raw_model_shader, glm::vec4(236, 193, 111, 255) / (255.0f), white_tex.get_texture_id(), variance_map_buffer.get_color_attachment(0));
    RawModelFlatColorMaterial flat_quad_mat(raw_model_flat_color_shader, glm::vec4(0.0, 1.0, 1.0, 1.0));

    RawModel cube_raw(cube_vertices, cube_indices, GL_STATIC_DRAW);
    RawModelMaterial shaded_cube_mat(raw_model_shader, glm::vec4(1.0, 1.0, 1.0, 1.0), white_tex.get_texture_id(), variance_map_buffer.get_color_attachment(0));
    RawModelFlatColorMaterial flat_cube_mat(raw_model_flat_color_shader, glm::vec4(0.0, 1.0, 0.0, 1.0));

    Texture2D color_palette_tex("color_palette.png");
    RawModel garage_raw("garage.fbx");
    RawModelMaterial shaded_garage_mat(raw_model_shader, glm::vec4(1.0, 1.0, 1.0, 1.0), color_palette_tex.get_texture_id(), variance_map_buffer.get_color_attachment(0));
    RawModelFlatColorMaterial flat_garage_mat(raw_model_flat_color_shader, glm::vec4(1.0, 0.0, 1.0, 1.0));

    Texture2D wood_workbench_tex("carpenterbench_albedo.png");
    RawModel wood_workbench_raw("wood_workbench.fbx");
    RawModelMaterial shaded_workbench_mat(raw_model_shader, glm::vec4(1.0, 1.0, 1.0, 1.0), wood_workbench_tex.get_texture_id(), variance_map_buffer.get_color_attachment(0));
    RawModelFlatColorMaterial flat_workbench_mat(raw_model_flat_color_shader, glm::vec4(1.0, 1.0, 0.0, 1.0));

    Texture2D container_tex("container_albedo.png");
    RawModel container_raw("container.fbx");
    RawModelMaterial shaded_container_mat(raw_model_shader, glm::vec4(1.0, 1.0, 1.0, 1.0), container_tex.get_texture_id(), variance_map_buffer.get_color_attachment(0));
    RawModelFlatColorMaterial flat_container_mat(raw_model_flat_color_shader, glm::vec4(0.0, 0.0, 1.0, 1.0));

    RawModel bunny_raw("stanford-bunny.fbx");
    RawModelMaterial shaded_bunny_mat(raw_model_shader, glm::vec4(1.0, 1.0, 1.0, 1.0), white_tex.get_texture_id(), variance_map_buffer.get_color_attachment(0));
    RawModelFlatColorMaterial flat_bunny_mat(raw_model_flat_color_shader, glm::vec4(0.0, 0.0, 1.0, 1.0));

    Entity ground_plane = Entity::Create("Ground plane");
    {
        ground_plane.AddComponent<TransformComponent>(glm::rotate(-glm::half_pi<float>(), glm::vec3(1.0, 0.0, 0.0)) * glm::scale(glm::vec3(500, 500, 1)) * glm::mat4(1.0));
        ground_plane.AddComponent<QuadRendererComponent>(&quad_raw);
        Material* materials[2] = { &shaded_quad_mat, &flat_quad_mat };
        ground_plane.AddComponent<MaterialComponent>(materials);
    }

    Entity wall = Entity::Create("Wall");
    {
        wall.AddComponent<TransformComponent>(glm::translate(glm::vec3(0.0, 2.0, -8.0)) * glm::scale(glm::vec3(25, 3, 1)) * glm::mat4(1.0));
        wall.AddComponent<QuadRendererComponent>(&quad_raw);
        Material* materials[2] = { &shaded_quad_mat, &flat_quad_mat };
        wall.AddComponent<MaterialComponent>(materials);
    }

    Entity workbench = Entity::Create("Workbench");
    {
        workbench.AddComponent<TransformComponent>(glm::translate(glm::vec3(-8.0, 1.1, 0.0))* glm::rotate(glm::quarter_pi<float>(), glm::vec3(0, 1, 0))* glm::mat4(1.0));
        workbench.AddComponent<ModelRendererComponent>(&wood_workbench_raw);
        Material* materials[2] = { &shaded_workbench_mat, &flat_workbench_mat };
        workbench.AddComponent<MaterialComponent>(materials);
    }

    Entity bunny = Entity::Create("Bunny");
    {
        bunny.AddComponent<TransformComponent>(glm::translate(glm::vec3(-8.0, 20.0, 0.0)) * glm::rotate(glm::quarter_pi<float>() / 2.0f, glm::vec3(1, 0, 0)) * glm::mat4(1.0));
        bunny.AddComponent<ModelRendererComponent>(&bunny_raw);
        Material* materials[2] = { &shaded_bunny_mat, &flat_bunny_mat };
        bunny.AddComponent<MaterialComponent>(materials);
    }

    Entity container = Entity::Create("Container");
    {
        container.AddComponent<TransformComponent>(glm::rotate(glm::half_pi<float>(), glm::vec3(0, 1, 0))* glm::scale(glm::vec3(1, 1, 1))* glm::mat4(1.0));
        container.AddComponent<ModelRendererComponent>(&container_raw);
        Material* materials[2] = { &shaded_container_mat, &flat_container_mat };
        container.AddComponent<MaterialComponent>(materials);
    }

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

    for (int i = 0; i < garage_positions.size(); i++)
    {
        Entity garage = Entity::Create("Garage");
        {
            garage.AddComponent<TransformComponent>(glm::translate(garage_positions[i]) * glm::rotate(-glm::half_pi<float>(), glm::vec3(0, 1, 0)) * glm::scale(garage_sizes[i]) * glm::mat4(1.0));
            garage.AddComponent <ModelRendererComponent > (&garage_raw);
            Material* materials[2] = { &shaded_garage_mat, &flat_garage_mat };
            garage.AddComponent<MaterialComponent>(materials);
        }
    }

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
    particle_system.update(dt, *particle_cs_shader);

    glm::mat4 light_view = glm::lookAt(directional_light * ortho_size, glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
    glm::mat4 light_view_projection = glm::ortho<float>(-ortho_size, ortho_size, -ortho_size, ortho_size, 0.1, ortho_far) * light_view;
    /** UPDATE END **/

    /* DRAW SCENE TO SHADOW MAP */
    shadow_map_buffer.bind();
    GL_CHECK(glClearColor(0.0, 0.0, 0.0, 1.0));
    GL_CHECK(glClear(GL_DEPTH_BUFFER_BIT));
    GL_CHECK(glEnable(GL_DEPTH_TEST));
    {
      environment_settings.camera_view_projection = light_view_projection;
      draw_models(MaterialIndex::FLAT, environment_settings);
    }
    shadow_map_buffer.unbind();

    /* RENDER VARIANCE SHADOW MAP */
    variance_map_buffer.bind();
    GL_CHECK(glClearColor(0.0, 0.0, 0.0, 0.0));
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT));
    GL_CHECK(glDisable(GL_DEPTH_TEST));

    variance_shadow_map_shader->bind();
    variance_shadow_map_shader->set_int("u_DepthTexture", 0);
    static GLuint empty_vao = 0;
    if (!empty_vao)
        GL_CHECK(glGenVertexArrays(1, &empty_vao));
    GL_CHECK(glBindVertexArray(empty_vao));

    GL_CHECK(glActiveTexture(GL_TEXTURE0));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, shadow_map_buffer.get_depth_attachment()));
    GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, 6));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, 0));

    GL_CHECK(glBindVertexArray(0));
    variance_map_buffer.unbind();

    /* DRAW SCENE TO BACKBUFFER */
    frame_buffer.bind();
    GL_CHECK(glClearColor(0.0, 0.0, 0.0, 1.0));
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
    GL_CHECK(glEnable(GL_DEPTH_TEST));
    /** SKYBOX RENDERING BEGIN, drawn early to enable transparent objects **/
    if (draw_skybox_b) {
      environment_settings.camera_view_projection = camera.get_view_projection(false);
      draw_skybox(*skybox_shader, skyboxes[current_skybox_idx], environment_settings);
    }
    /** SKYBOX RENDERING END **/

    environment_settings.camera_view_projection = camera.get_view_projection(true);
    environment_settings.directional_light = directional_light;
    environment_settings.light_view_projection = light_view_projection;
    draw_models(MaterialIndex::SHADED, environment_settings);

    static bool mouse_pressed = false;
    if (Input::IsMousePressed(Button::LEFT))
        mouse_pressed = true;

    else if (mouse_pressed && !Input::IsMousePressed(Button::LEFT))
    {
        mouse_pressed = false;
        glm::dvec2 mouse_pos;
        Input::GetCursor(mouse_pos);
        glReadBuffer(GL_COLOR_ATTACHMENT1);
        uint32_t entity_id;
        glReadPixels((int)mouse_pos.x, (int)(window.get_height() - mouse_pos.y), 1, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, &entity_id);
        if (Entity::Exists(entity_id))
            g_selected_entity = Entity::Get(entity_id);
    }

    if (draw_colliders)
    {
      for (auto& collider : colliders)
      {
        GL_CHECK(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));
        GL_CHECK(glDisable(GL_CULL_FACE));
        //cube_model.material_index = MaterialIndex::FLAT;
        //cube_model.transform = glm::translate((collider.max + collider.min) / 2.0f) * glm::scale(collider.max - collider.min) * glm::mat4(1.0);
        //Renderer::draw(cube_model, environment_settings);
        GL_CHECK(glEnable(GL_CULL_FACE));
        GL_CHECK(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
      }
    }

    if (g_DrawGrass) {
        grass_shader->bind();
        // Grass VS Uniforms
        grass_shader->set_matrix4fv("u_ViewMatrix", &camera.get_view_matrix(true)[0][0]);
        grass_shader->set_matrix4fv("u_ProjectionMatrix", &camera.get_projection_matrix()[0][0]);
        glm::vec3 camera_pos = camera.get_position();
        grass_shader->set_float3v("u_CameraPosition", 1, &camera.get_position()[0]);
        grass_shader->set_float3("u_SystemBoundsMin", bbox_min.x, 0, bbox_min.z);
        grass_shader->set_float3("u_SystemBoundsMax", bbox_max.x, 0, bbox_max.z);
        grass_shader->set_int3("u_ParticlesPerDim", grass_per_dim.x, 1, grass_per_dim.y);
        grass_shader->set_float("u_Time", time);
        grass_shader->set_float("u_WindAmp", u_WindAmp);
        grass_shader->set_float("u_WindFactor", u_WindFactor);
        grass_shader->set_float2("u_WindDirection", u_WindDirection.x, u_WindDirection.y);
        grass_shader->set_int("u_WindTexture", 0);
        grass_shader->set_int("u_VerticesPerBlade", vertices_per_blade);
        noise_marble_tex.bind(0);

        // Grass FS Uniforms
        grass_shader->set_float3("u_LightDirection", directional_light.x, directional_light.y, directional_light.z);
        grass_shader->set_matrix4fv("u_LightViewProjection", &environment_settings.light_view_projection[0][0]);
        grass_shader->set_int("u_ShadowMap", 2);
        GL_CHECK(glActiveTexture(GL_TEXTURE2));
        GL_CHECK(glBindTexture(GL_TEXTURE_2D, shadow_map_buffer.get_depth_attachment()));

        grass_system.draw_instanced(vertices_per_blade);
        grass_shader->unbind();
    }

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
        GLuint rendered_texture = frame_buffer.get_color_attachment(0);
        if (draw_shadow_map)
            rendered_texture = variance_map_buffer.get_color_attachment(0);
        if (draw_depthbuffer)
            rendered_texture = frame_buffer.get_depth_attachment();
        GL_CHECK(glClearColor(0.0, 0.0, 1.0, 1.0));
        GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
        GL_CHECK(glDisable(GL_DEPTH_TEST));
        draw_to_screen(*framebuffer_shader, rendered_texture);
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
    draw_gui(camera);
    /** GUI RENDERING END **/

    camera.set_movement_speed(movement_speed);
    camera.set_rotation_speed(rotation_speed);

    window.resize(camera);
    window.swap_buffers ();
    window.poll_events ();
  }

#ifdef __APPLE__
  ImGui_ImplMetal_Shutdown();
#else
  ImGui_ImplOpenGL3_Shutdown();
#endif
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  //pDevice->release();

  return 0;
}

void update(const Window& window, double dt, Camera& camera) {
  if (Input::IsKeyPressed(Key::LEFT)) camera.rotate_yaw(dt);
  if (Input::IsKeyPressed(Key::RIGHT)) camera.rotate_yaw(-dt);
  if (Input::IsKeyPressed(Key::UP)) camera.rotate_pitch(dt);
  if (Input::IsKeyPressed(Key::DOWN)) camera.rotate_pitch(-dt);

  int direction = 0;
  if (Input::IsKeyPressed(Key::W)) direction |= Camera::FORWARD;
  if (Input::IsKeyPressed(Key::S)) direction |= Camera::BACKWARD;
  if (Input::IsKeyPressed(Key::A)) direction |= Camera::LEFT;
  if (Input::IsKeyPressed(Key::D)) direction |= Camera::RIGHT;
  camera.move(dt, direction);

  if (Input::IsKeyClicked(Key::P)) { simulation_pause = !simulation_pause; }
}

void draw_models(MaterialIndex index, const EnvironmentSettings& settings)
{
    if (draw_quads)
    {
        GL_CHECK(glDepthMask(depth_cull || quad_alpha >= 1.0f ? GL_TRUE : GL_FALSE));
        GL_CHECK(glDisable(GL_CULL_FACE));
        Entity::ForEachWith<TransformComponent, QuadRendererComponent, MaterialComponent>(
            [&](Entity entity) {
                auto& tc = entity.GetComponent<TransformComponent>();
                auto& qrc = entity.GetComponent<QuadRendererComponent>();
                auto& matc = entity.GetComponent<MaterialComponent>();
                Renderer::draw_model(entity.GetID(), tc.transform, qrc.model, matc.materials[index], settings);
            }
        );
        GL_CHECK(glEnable(GL_CULL_FACE));
        GL_CHECK(glDepthMask(GL_TRUE));
    }

    Entity::ForEachWith<TransformComponent, ModelRendererComponent, MaterialComponent>(
        [&](Entity entity) {
            auto& tc = entity.GetComponent<TransformComponent>();
            auto& mrc = entity.GetComponent<ModelRendererComponent>();
            auto& matc = entity.GetComponent<MaterialComponent>();
            Renderer::draw_model(entity.GetID(), tc.transform, mrc.model, matc.materials[index], settings);
        }
    );
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

void draw_gui(const Camera& camera)
{
    ImGuiIO& io = ImGui::GetIO();
    float* view_matrix = (float*)&camera.get_view_matrix(true)[0];
    float* proj_matrix = (float*)&camera.get_projection_matrix()[0];

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuizmo::BeginFrame();
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

    ImGui::Begin("Settings panel");
    ImGui::Text("FPS: %f, time: %f (ms)", (1 / fps_sum), fps_sum * 1000);
    ImGui::Text("Update-time: %f (ms)", update_sum * 1000);
    ImGui::Text("Draw-time: %f (ms)", draw_sum * 1000);

    ImGui::Dummy(ImVec2(0.0, 15.0));
    ImGui::Text("Shaders");
    if (ImGui::Button("Reload"))
        ShaderManager::Reload();

    ImGui::Dummy(ImVec2(0.0, 5.0));
    if(ImGui::CollapsingHeader("ImGuizmo"))
    {            
        static bool draw_grid = false;
        ImGui::Checkbox("Draw grid", &draw_grid);
        if (draw_grid)
            ImGuizmo::DrawGrid(view_matrix, proj_matrix, (float*)&glm::mat4(1.0f)[0], 100.f);
    }

    // Draw Gizmo
    {
        static ImGuizmo::OPERATION s_ImGuizmoOperation = ImGuizmo::TRANSLATE;
        if (Input::IsKeyPressed(Key::T))
            s_ImGuizmoOperation = ImGuizmo::TRANSLATE;
        if (Input::IsKeyPressed(Key::R))
            s_ImGuizmoOperation = ImGuizmo::ROTATE;
        if (Input::IsKeyPressed(Key::E))
            s_ImGuizmoOperation = ImGuizmo::SCALE;

        bool object_selected = g_selected_entity.GetID() != Entity::Invalid().GetID();
        if (object_selected)
        {
            ImGui::Text("Selected model id: %d", g_selected_entity.GetID());
            TransformComponent& tc = g_selected_entity.GetComponent<TransformComponent>();
            ImGuizmo::Manipulate(view_matrix, proj_matrix, s_ImGuizmoOperation, ImGuizmo::WORLD, (float*)&tc.transform[0], NULL, NULL, NULL, NULL);
        }
        else
        {
            ImGui::Text("Selected model id: None");
        }
        ImGuizmo::Enable(object_selected);
    }

    ImGui::Dummy(ImVec2(0.0, 5.0));
    if (ImGui::CollapsingHeader("Simulation"))
    {
        ImGui::Text("Num particles: %d, (%d x %d x %d)",
        particles_per_dim.x * particles_per_dim.y * particles_per_dim.z,
        particles_per_dim.x, particles_per_dim.y, particles_per_dim.z);
        ImGui::Dummy(ImVec2(0.0, 5.0));
        ImGui::Checkbox("Colored particles", &colored_particles);
    }

    ImGui::Dummy(ImVec2(0.0, 5.0));
    if (ImGui::CollapsingHeader("Environment"))
    {
        ImGui::Checkbox("Draw depthbuffer", &draw_depthbuffer);
        ImGui::Checkbox("Depth cull", &depth_cull);
        ImGui::Checkbox("Draw quad", &draw_quads);
        if (draw_quads)
            ImGui::SliderFloat("Quad alpha", &quad_alpha, 0.0, 1.0);
        ImGui::Checkbox("Draw colliders", &draw_colliders);
    }

    ImGui::Dummy(ImVec2(0.0, 5.0));
    if (ImGui::CollapsingHeader("Grass")) 
    {
        ImGui::Checkbox("Enable", &g_DrawGrass);

        ImGui::Dummy(ImVec2(0.0, 5.0));
        ImGui::SliderFloat2("Wind direction", (float*) &u_WindDirection, -1.0, 1.0);
        ImGui::SliderFloat("WindAmp", &u_WindAmp, 0.0, 1.0);
        ImGui::SliderFloat("WindFactor", &u_WindFactor, 0.0, 40.0);

        ImGui::Dummy(ImVec2(0.0, 5.0));
        ImGui::SliderInt("Vertices Per Blade", &vertices_per_blade, 4.0, 16.0);
        vertices_per_blade = (vertices_per_blade / 2) * 2;
    }

    ImGui::Dummy(ImVec2(0.0, 5.0));
    if (ImGui::CollapsingHeader("Lighting"))
    {
        ImGui::Dummy(ImVec2(0.0, 5.0));
        ImGui::SliderFloat3("Directional Light", &directional_light[0], -1, 1);
        directional_light = glm::normalize(directional_light);
        ImGui::SliderFloat("Ortho size", &ortho_size, 1, 1000);
        ImGui::SliderFloat("Ortho far", &ortho_far, 1, 1000);
        ImGui::Checkbox("Draw shadow map", &draw_shadow_map);
    }

    ImGui::Dummy(ImVec2(0.0, 5.0));
    if (ImGui::CollapsingHeader("Skybox"))
    {
        ImGui::Checkbox("Draw skybox", &draw_skybox_b);
        if (ImGui::BeginCombo("##SkyboxTexture", skybox_combo_label))
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
    }

    ImGui::Dummy(ImVec2(0.0, 5.0));
    if(ImGui::CollapsingHeader("Camera"))
    {
        ImGui::SliderFloat("Movement speed", &movement_speed, 1.0, 10.0);
        ImGui::SliderFloat("Rotation speed", &rotation_speed, 1.0, 200.0);
    }

    ImGui::End(); // Settings panel

    ImGui::Begin("Hierarchy");
    Entity::ForEachWith<NameComponent>([](Entity entity) {
        std::string& name = entity.GetComponent<NameComponent>().name;
        ImGui::PushItemWidth(-1.0f);
        if (ImGui::Button(name.c_str(), ImVec2(ImGui::GetWindowSize().x, 0.0f)))
            g_selected_entity = entity;
        ImGui::PopItemWidth();
    });
    ImGui::End(); // Hierarchy

    ImGui::Begin("Inspector");
    if (g_selected_entity.GetID() != Entity::Invalid().GetID())
    {
        std::string& name = g_selected_entity.GetComponent<NameComponent>().name;
        ImGui::Text("Model name: %s", name.c_str());
        ImGui::Text("Id: %d", g_selected_entity.GetID());
        ImGui::Text("Transform");
        ImGui::Text("Material");
        ImGui::Text("Whatever");
    }
    ImGui::End(); // Inspector

    ImGui::Render();
#ifdef __APPLE__
    ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), pCmdBuffer, pEnc);
#else
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif
}