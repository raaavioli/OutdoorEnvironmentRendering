#pragma once

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

#include <entt/entity/registry.hpp>

#include <entity.h>
#include <input.h>
#include <camera.h>
#include "gl_helpers.h"
#include "framebuffer.h"
#include "window.h"
#include "particlesystem.h"

class Scene
{
	friend Entity;

public:
	Scene(const Window& window, const std::string& name);
	~Scene() = default;

	/**
	* Update all systems
	*/
	void Update();

	/**
	* Dispatch compute and draw
	*/
	void Draw(const Camera& camera, const Window& window);

	/**
	* Draw ImGui
	*/
	void DrawGUI(const Camera& camera);

	Entity CreateEntity(const std::string& name);
	void DestroyEntity(Entity entity);

	FrameBuffer* m_VarianceMapBuffer;

private:
	template <typename Component>
	void DrawComponentUIIfExists(Entity entity);

private:
	std::string m_Name;
	entt::registry m_EntityRegistry;

	Entity m_ActiveEntity;

	// Textures received from: https://www.humus.name/index.php?page=Textures
	const char* skyboxes_names[3] = { "skansen", "ocean", "church" };
	int current_skybox_idx = 1;
	const char* skybox_combo_label = skyboxes_names[current_skybox_idx];
	Skybox* m_Skyboxes[3];

	EnvironmentSettings environment_settings;

	FrameBuffer* m_DefaultFrameBuffer;
	FrameBuffer* m_ShadowMapBuffer;

	Shader* m_GrassShader;
	Shader* m_SkyboxShader;
	Shader* m_ParticleShader;
	Shader* m_ParticleCSShader;
	Shader* m_FramebufferShader;
	Shader* m_VarianceShadowMapShader;

	Texture2D noise_marble_tex;

	/** SETTINGS VARIABLES **/
	bool draw_shadow_map = false;
	bool draw_colliders = true;
	bool colored_particles = false;
	bool depth_cull = false;
	bool draw_quads = true;
	bool draw_skybox_b = true;
	bool draw_depthbuffer = false;

	// Wind
	float u_WindAmp = 0.3f;
	float u_WindFactor = 20.0f;
	glm::vec2 u_WindDirection = glm::vec2(1.0, 1.0);

	// Grass
	bool g_DrawGrass = true;
	int32_t vertices_per_blade = 8;
	glm::vec3 bbox_scale = glm::vec3(2, 1, 2) * 100.0f;
	glm::vec3 bbox_center = glm::vec3(0, bbox_scale.y / 2 - 1, 0);
	glm::vec3 bbox_min = bbox_center + glm::vec3(-0.5, -0.5, -0.5) * bbox_scale;
	glm::vec3 bbox_max = bbox_center + glm::vec3(0.5, 0.5, 0.5) * bbox_scale;
	glm::ivec2 grass_per_dim = glm::ivec2(3000, 3000);
	ParticleSystem grass_system;

	float quad_alpha = 1.0;
	float ortho_size = 25.0f;
	float ortho_far = 100.0f;

	glm::ivec3 particles_per_dim = glm::ivec3(160, 160 * 2, 160);
	glm::vec3 directional_light = glm::normalize(glm::vec3(-1.0, 1.0, -1.0));
	glm::mat4 light_view = glm::lookAt(directional_light * ortho_size, glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
	glm::mat4 light_view_projection = glm::ortho<float>(-ortho_size, ortho_size, -ortho_size, ortho_size, 0.1, ortho_far) * light_view;
};
