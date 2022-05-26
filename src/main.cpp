#include<iostream>
#include<fstream>
#include<vector>
#include<string>
#include<map>

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
#include "scene.h"

Entity g_selected_entity = Entity::Invalid();


bool simulation_pause = false;
double update_sum = 0.0;
double draw_sum = 0.0;
double fps_sum = 0.0;

float movement_speed = 15.0;
float rotation_speed = 100.0;




struct AABB
{
  glm::vec3 min;
  glm::vec3 max;
};

/** FUNCTIONS */
void update(const Window& window, double dt, Camera& camera);
void draw_gui();

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

    int work_group_sizes[3];
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_group_sizes[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_group_sizes[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_group_sizes[2]);
    std::cout << "GL_MAX_COMPUTE_WORK_GROUP_SIZE: " << work_group_sizes[0] << ", " << work_group_sizes[1] << ", " << work_group_sizes[2] << std::endl;

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

    Scene testScene(window, "TestScene");

    Shader* raw_model_shader = ShaderManager::Create("raw_model.glsl");
    Shader* raw_model_flat_color_shader = ShaderManager::Create("raw_model_flat_color.glsl");

    Texture2D white_tex;
    RawModel quad_raw(quad_vertices, quad_indices, GL_STATIC_DRAW);
    RawModelMaterial shaded_quad_mat(raw_model_shader, glm::vec4(236, 193, 111, 255) / (255.0f), white_tex.get_texture_id(), testScene.m_VarianceMapBuffer->get_color_attachment(0));
    RawModelFlatColorMaterial flat_quad_mat(raw_model_flat_color_shader, glm::vec4(0.0, 1.0, 1.0, 1.0));

    RawModel cube_raw(cube_vertices, cube_indices, GL_STATIC_DRAW);
    RawModelMaterial shaded_cube_mat(raw_model_shader, glm::vec4(1.0, 1.0, 1.0, 1.0), white_tex.get_texture_id(), testScene.m_VarianceMapBuffer->get_color_attachment(0));
    RawModelFlatColorMaterial flat_cube_mat(raw_model_flat_color_shader, glm::vec4(0.0, 1.0, 0.0, 1.0));

    Texture2D color_palette_tex("color_palette.png");
    RawModel garage_raw("garage.fbx");
    RawModelMaterial shaded_garage_mat(raw_model_shader, glm::vec4(1.0, 1.0, 1.0, 1.0), color_palette_tex.get_texture_id(), testScene.m_VarianceMapBuffer->get_color_attachment(0));
    RawModelFlatColorMaterial flat_garage_mat(raw_model_flat_color_shader, glm::vec4(1.0, 0.0, 1.0, 1.0));

    Texture2D wood_workbench_tex("carpenterbench_albedo.png");
    RawModel wood_workbench_raw("wood_workbench.fbx");
    RawModelMaterial shaded_workbench_mat(raw_model_shader, glm::vec4(1.0, 1.0, 1.0, 1.0), wood_workbench_tex.get_texture_id(), testScene.m_VarianceMapBuffer->get_color_attachment(0));
    RawModelFlatColorMaterial flat_workbench_mat(raw_model_flat_color_shader, glm::vec4(1.0, 1.0, 0.0, 1.0));

    Texture2D container_tex("container_albedo.png");
    RawModel container_raw("container.fbx");
    RawModelMaterial shaded_container_mat(raw_model_shader, glm::vec4(1.0, 1.0, 1.0, 1.0), container_tex.get_texture_id(), testScene.m_VarianceMapBuffer->get_color_attachment(0));
    RawModelFlatColorMaterial flat_container_mat(raw_model_flat_color_shader, glm::vec4(0.0, 0.0, 1.0, 1.0));

    RawModel bunny_raw("stanford-bunny.fbx");
    RawModelMaterial shaded_bunny_mat(raw_model_shader, glm::vec4(1.0, 1.0, 1.0, 1.0), white_tex.get_texture_id(), testScene.m_VarianceMapBuffer->get_color_attachment(0));
    RawModelFlatColorMaterial flat_bunny_mat(raw_model_flat_color_shader, glm::vec4(0.0, 0.0, 1.0, 1.0));

    Entity ground_plane = testScene.CreateEntity("Ground plane");
    {
        ground_plane.GetComponent<TransformComponent>().transform = glm::rotate(-glm::half_pi<float>(), glm::vec3(1.0, 0.0, 0.0)) * glm::scale(glm::vec3(500, 500, 1)) * glm::mat4(1.0);
        ground_plane.AddComponent<QuadRendererComponent>(&quad_raw);
        MaterialComponent& mc = ground_plane.AddComponent<MaterialComponent>();
        mc.materials.push_back(&shaded_quad_mat);
        mc.materials.push_back(&flat_quad_mat);
    }

    Entity wall = testScene.CreateEntity("Wall");
    {
        wall.GetComponent<TransformComponent>().transform = glm::translate(glm::vec3(0.0, 2.0, -8.0)) * glm::scale(glm::vec3(25, 3, 1)) * glm::mat4(1.0);
        wall.AddComponent<QuadRendererComponent>(&quad_raw);
        MaterialComponent& mc = wall.AddComponent<MaterialComponent>();
        mc.materials.push_back(&shaded_quad_mat);
        mc.materials.push_back(&flat_quad_mat);
    }

    Entity workbench = testScene.CreateEntity("Workbench");
    {
        workbench.GetComponent<TransformComponent>().transform = glm::translate(glm::vec3(-8.0, 1.1, 0.0)) * glm::rotate(glm::quarter_pi<float>(), glm::vec3(0, 1, 0)) * glm::mat4(1.0);
        workbench.AddComponent<ModelRendererComponent>(&wood_workbench_raw);
        MaterialComponent& mc = workbench.AddComponent<MaterialComponent>();
        mc.materials.push_back(&shaded_workbench_mat);
        mc.materials.push_back(&flat_workbench_mat);
    }

    Entity bunny = testScene.CreateEntity("Bunny");
    {
        bunny.GetComponent<TransformComponent>().transform = glm::translate(glm::vec3(-8.0, 20.0, 0.0)) * glm::rotate(glm::quarter_pi<float>() / 2.0f, glm::vec3(1, 0, 0)) * glm::mat4(1.0);
        bunny.AddComponent<ModelRendererComponent>(&bunny_raw);
        MaterialComponent& mc = bunny.AddComponent<MaterialComponent>();
        mc.materials.push_back(&shaded_bunny_mat);
        mc.materials.push_back(&flat_bunny_mat);
    }

    Entity container = testScene.CreateEntity("Container");
    {
        container.GetComponent<TransformComponent>().transform = glm::rotate(glm::half_pi<float>(), glm::vec3(0, 1, 0)) * glm::scale(glm::vec3(1, 1, 1)) * glm::mat4(1.0);
        container.AddComponent<ModelRendererComponent>(&container_raw);
        MaterialComponent& mc = container.AddComponent<MaterialComponent>();
        mc.materials.push_back(&shaded_container_mat);
        mc.materials.push_back(&flat_container_mat);
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
        Entity garage = testScene.CreateEntity("Garage");
        {
            garage.GetComponent<TransformComponent>().transform = glm::translate(garage_positions[i]) * glm::rotate(-glm::half_pi<float>(), glm::vec3(0, 1, 0)) * glm::scale(garage_sizes[i]) * glm::mat4(1.0);
            garage.AddComponent <ModelRendererComponent > (&garage_raw);
            MaterialComponent& mc = garage.AddComponent<MaterialComponent>();
            mc.materials.push_back(&shaded_garage_mat);
            mc.materials.push_back(&flat_garage_mat);
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

    static float time = 0.0f;

  while (!window.should_close ()) {
    /** UPDATE BEGIN **/
    double dt = simulation_pause ? 0 : clock.tick();
    time += dt;
    fps[n++ % fps_wrap] = dt;
    update(window, dt, camera);
    testScene.Update();
    /** UPDATE END **/

    testScene.Draw(camera, window);

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
    testScene.DrawGUI(camera);
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

void draw_gui()
{
    ImGui::Begin("Settings");
    ImGui::Text("FPS: %f, time: %f (ms)", (1 / fps_sum), fps_sum * 1000);
    ImGui::Text("Update-time: %f (ms)", update_sum * 1000);
    ImGui::Text("Draw-time: %f (ms)", draw_sum * 1000);

    ImGui::Dummy(ImVec2(0.0, 15.0));
    ImGui::Text("Shaders");
    if (ImGui::Button("Reload"))
        ShaderManager::Reload();

    ImGui::Dummy(ImVec2(0.0, 5.0));
    if (ImGui::CollapsingHeader("Camera"))
    {
        ImGui::SliderFloat("Movement speed", &movement_speed, 1.0, 10.0);
        ImGui::SliderFloat("Rotation speed", &rotation_speed, 1.0, 200.0);
    }
    ImGui::End(); // Settings
}