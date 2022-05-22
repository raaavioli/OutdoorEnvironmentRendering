#include "material.h"

#include "gl_helpers.h"

void RawModelMaterial::bind(uint32_t model_id, glm::mat4& transform, const EnvironmentSettings& settings)
{
	shader->bind();
	shader->set_matrix4fv("u_Model", &transform[0][0]);
	shader->set_matrix4fv("u_ViewProjection", &settings.camera_view_projection[0][0]);
	shader->set_matrix4fv("u_LightViewProjection", &settings.light_view_projection[0][0]);
	shader->set_float3("u_DirectionalLight", settings.directional_light.x, settings.directional_light.y, settings.directional_light.z);

	shader->set_float4("u_ModelColor", u_ModelColor.r, u_ModelColor.g, u_ModelColor.b, u_ModelColor.a);
	shader->set_uint("u_GUID", model_id);
	shader->set_int("u_Texture", 0);
	GL_CHECK(glActiveTexture(GL_TEXTURE0));
	GL_CHECK(glBindTexture(GL_TEXTURE_2D, u_Texture));
	shader->set_int("u_ShadowMap", 1);
	GL_CHECK(glActiveTexture(GL_TEXTURE1));
	GL_CHECK(glBindTexture(GL_TEXTURE_2D, u_ShadowMap));
};

void RawModelMaterial::unbind()
{
	shader->unbind();
	GL_CHECK(glActiveTexture(GL_TEXTURE1));
	GL_CHECK(glBindTexture(GL_TEXTURE_2D, 0));
	GL_CHECK(glActiveTexture(GL_TEXTURE0));
	GL_CHECK(glBindTexture(GL_TEXTURE_2D, 0));
};

void RawModelFlatColorMaterial::bind(uint32_t model_id, glm::mat4& transform, const EnvironmentSettings& settings)
{
	shader->bind();
	shader->set_matrix4fv("u_Model", &transform[0][0]);
	shader->set_matrix4fv("u_ViewProjection", &settings.camera_view_projection[0][0]);

	shader->set_float4("u_ModelColor", u_ModelColor.r, u_ModelColor.g, u_ModelColor.b, u_ModelColor.a);
};

void RawModelFlatColorMaterial::unbind()
{
	shader->unbind();
};