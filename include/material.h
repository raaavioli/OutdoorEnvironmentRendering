#pragma once

#include <glm/glm.hpp>
#include <imgui.h>

#include "assets.h"
#include "gl_helpers.h"

struct EnvironmentSettings
{
	glm::mat4 camera_view_projection;
	glm::vec3 directional_light;
	glm::mat4 light_view_projection;
	uint32_t shadow_map;
};


struct Material
{
	virtual void bind(uint32_t model_id, glm::mat4& transform, const EnvironmentSettings& settings) = 0;
	virtual void unbind() = 0;

protected:
	Shader* shader;
};

// TODO: Autogenerate these based on material uniforms in shaders
struct ExampleMaterial
{
	// Material properties
	glm::vec3 _Color;
	// Samplers
	GLuint _Albedo = -1;

	void Bind(int sampler_index) 
	{
		Shader* shader = ExampleMaterial::GetShader();
		shader->bind();
		shader->set_float3("u_Color", _Color.x, _Color.y, _Color.z);
		if (_Albedo != -1)
		{
			shader->set_int("u_AlbedoMap", sampler_index);
			GL_CHECK(glActiveTexture(GL_TEXTURE0 + sampler_index));
			GL_CHECK(glBindTexture(GL_TEXTURE_2D, _Albedo));
			sampler_index++;
		}

		/* TODO: Enable normal mapping
		if (_NormalMap != -1)
		{
			shader->set_int("u_NormalMap", sampler_index);
			GL_CHECK(glActiveTexture(GL_TEXTURE0 + sampler_index));
			GL_CHECK(glBindTexture(GL_TEXTURE_2D, _Albedo));
			sampler_index++;
		}
		*/
	}

	void DrawUI()
	{
		if (ImGui::BeginTable("Material-table", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
		{
			ImGui::TableSetupColumn("One");
			ImGui::TableSetupColumn("Two", ImGuiTableColumnFlags_WidthFixed, 128);

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::ColorEdit3("Color", &_Color[0]);

			ImGui::TableNextRow();
			if (_Albedo != -1)
			{
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("AlbedoMap"); 
				ImGui::Text("Name: %s", "filename.png");
				
				ImGui::TableSetColumnIndex(1);
				ImVec2 pos = ImGui::GetCursorScreenPos();
				ImVec2 size = ImVec2(128, 128);
				ImVec2 extent = ImVec2(5, 5);
				static bool selected = false;
				if (selected = ImGui::Selectable("##Button", selected, 0, size))
				{
					Texture2D* texture = AssetManager::LoadTextureFromFileSystem();
					_Albedo = texture->get_texture_id();

				}
				ImGui::GetWindowDrawList()->AddImage((void*)(intptr_t)_Albedo,
					ImVec2(pos.x + extent.x, pos.y + extent.y),
					ImVec2(pos.x + size.x - extent.x, pos.y + size.y - extent.x));
			}
			ImGui::EndTable();
		}
	}

	static Shader* GetShader() { return AssetManager::GetShader("example_material_shader.glsl"); }
};


struct RawModelMaterial : Material
{
	RawModelMaterial(Shader* shader, glm::vec4 model_color, GLuint texture, GLuint shadow_map) : u_ModelColor(model_color), u_Texture(texture), u_ShadowMap(shadow_map)
	{
		this->shader = shader;
	}

	void bind(uint32_t model_id, glm::mat4& transform, const EnvironmentSettings& settings) override;
	void unbind() override;

	glm::vec4 u_ModelColor;
	
	GLuint u_Texture;
	GLuint u_ShadowMap;	
};

struct RawModelFlatColorMaterial : Material
{
	RawModelFlatColorMaterial(Shader* shader, glm::vec4 model_color) : u_ModelColor(model_color) 
	{
		this->shader = shader;
	}

	void bind(uint32_t model_id, glm::mat4& transform, const EnvironmentSettings& settings) override;
	void unbind() override;

	glm::vec4 u_ModelColor;
};