#include "scene.h"

#include "renderer.h"

Scene::Scene(const Window& window, const std::string& name) : 
    m_Name(name), 
    noise_marble_tex("noisemarble1.png"),
    grass_system(glm::ivec3(grass_per_dim.x, 1, grass_per_dim.y), glm::vec3(bbox_min.x, 0, bbox_min.z) / 32.0f, glm::vec3(bbox_max.x, 0, bbox_max.z) / 32.0f),
    m_ActiveEntity(Entity::Invalid())
{
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
    // TODO: Fix memory leak when creating new framebuffers
    m_DefaultFrameBuffer = new FrameBuffer(fb_cinfo);

    FrameBufferCreateInfo shadow_cinfo;
    shadow_cinfo.width = shadow_cinfo.height = 4096;
    shadow_cinfo.attachment_bits = AttachmentType::DEPTH;
    shadow_cinfo.num_color_attachments = 0;
    m_ShadowMapBuffer = new FrameBuffer(shadow_cinfo);

    FrameBufferCreateInfo vsm_cinfo;
    vsm_cinfo.width = vsm_cinfo.height = 4096;
    vsm_cinfo.attachment_bits = AttachmentType::COLOR;
    vsm_cinfo.num_color_attachments = 1;
    FrameBufferTextureCreateInfo vsm_tex_cinfo = { ColorFormat::RG16, GL_LINEAR, GL_LINEAR };
    vsm_cinfo.color_attachment_infos = &vsm_tex_cinfo;
    m_VarianceMapBuffer = new FrameBuffer(vsm_cinfo, true);

    m_GrassShader = ShaderManager::Create("grass.glsl");
    m_SkyboxShader = ShaderManager::Create("skybox.glsl");
    m_ParticleShader = ShaderManager::Create("particle.glsl");
    m_ParticleCSShader = ShaderManager::Create("particle_cs.glsl");
    m_FramebufferShader = ShaderManager::Create("framebuffer.glsl");
    m_VarianceShadowMapShader = ShaderManager::Create("variance_shadow_map.glsl");

    m_Skyboxes[0] = new Skybox(skyboxes_names[0], true);
    m_Skyboxes[1] = new Skybox(skyboxes_names[1], true);
    m_Skyboxes[2] = new Skybox(skyboxes_names[2], true);
    GL_CHECK(glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS));
}

void Scene::Update()
{
}

void Scene::Draw(const Camera& camera, const Window& window)
{
    /* DRAW SCENE TO SHADOW MAP */
    m_ShadowMapBuffer->bind();
    GL_CHECK(glClearColor(0.0, 0.0, 0.0, 1.0));
    GL_CHECK(glClear(GL_DEPTH_BUFFER_BIT));
    GL_CHECK(glEnable(GL_DEPTH_TEST));
    {
        environment_settings.camera_view_projection = light_view_projection;
        GL_CHECK(glDepthMask(depth_cull || quad_alpha >= 1.0f ? GL_TRUE : GL_FALSE));
        GL_CHECK(glDisable(GL_CULL_FACE));
        auto& quad_view = m_EntityRegistry.view<TransformComponent, QuadRendererComponent, MaterialComponent>();
        quad_view.each([&](auto entity, auto tc, auto qrc, auto matc) {
            Renderer::draw_model((uint32_t)entity, tc.transform, qrc.model, matc.materials[MaterialIndex::FLAT], environment_settings);
        });
        GL_CHECK(glEnable(GL_CULL_FACE));
        GL_CHECK(glDepthMask(GL_TRUE));

        auto& model_view = m_EntityRegistry.view<TransformComponent, ModelRendererComponent, MaterialComponent>();
        model_view.each([&](auto entity, auto tc, auto mrc, auto matc) {
            Renderer::draw_model((uint32_t)entity, tc.transform, mrc.model, matc.materials[MaterialIndex::FLAT], environment_settings);
        });
    }
    m_ShadowMapBuffer->unbind();

    /* RENDER VARIANCE SHADOW MAP */
    m_VarianceMapBuffer->bind();
    GL_CHECK(glClearColor(0.0, 0.0, 0.0, 0.0));
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT));
    GL_CHECK(glDisable(GL_DEPTH_TEST));

    m_VarianceShadowMapShader->bind();
    m_VarianceShadowMapShader->set_int("u_DepthTexture", 0);
    static GLuint empty_vao = 0;
    if (!empty_vao)
        GL_CHECK(glGenVertexArrays(1, &empty_vao));
    GL_CHECK(glBindVertexArray(empty_vao));

    GL_CHECK(glActiveTexture(GL_TEXTURE0));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, m_ShadowMapBuffer->get_depth_attachment()));
    GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, 6));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, 0));

    GL_CHECK(glBindVertexArray(0));
    m_VarianceMapBuffer->unbind();

    /* DRAW SCENE TO BACKBUFFER */
    m_DefaultFrameBuffer->bind();
    GL_CHECK(glClearColor(0.0, 0.0, 0.0, 1.0));
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
    GL_CHECK(glEnable(GL_DEPTH_TEST));
    /** SKYBOX RENDERING BEGIN, drawn early to enable transparent objects **/
    if (draw_skybox_b) {
        environment_settings.camera_view_projection = camera.get_view_projection(false);
        m_SkyboxShader->bind();
        m_SkyboxShader->set_matrix4fv("u_ViewProjection", &environment_settings.camera_view_projection[0][0]);
        m_SkyboxShader->set_int("cube_map", 0);

        m_Skyboxes[current_skybox_idx]->bind_cube_map(0);
        m_Skyboxes[current_skybox_idx]->draw();
        m_SkyboxShader->unbind();
    }
    /** SKYBOX RENDERING END **/

    {
    environment_settings.camera_view_projection = camera.get_view_projection(true);
    environment_settings.directional_light = directional_light;
    environment_settings.light_view_projection = light_view_projection;
    
        GL_CHECK(glDepthMask(depth_cull || quad_alpha >= 1.0f ? GL_TRUE : GL_FALSE));
        GL_CHECK(glDisable(GL_CULL_FACE));
        auto& quad_view = m_EntityRegistry.view<TransformComponent, QuadRendererComponent, MaterialComponent>();
        quad_view.each([&](auto entity, auto tc, auto qrc, auto matc) {
            Renderer::draw_model((uint32_t)entity, tc.transform, qrc.model, matc.materials[MaterialIndex::SHADED], environment_settings);
        });
        GL_CHECK(glEnable(GL_CULL_FACE));
        GL_CHECK(glDepthMask(GL_TRUE));

        auto& model_view = m_EntityRegistry.view<TransformComponent, ModelRendererComponent, MaterialComponent>();
        model_view.each([&](auto entity, auto tc, auto mrc, auto matc) {
            Renderer::draw_model((uint32_t)entity, tc.transform, mrc.model, matc.materials[MaterialIndex::SHADED], environment_settings);
        });

    }
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
        if (m_EntityRegistry.valid((entt::entity)entity_id))
            m_ActiveEntity = Entity(entity_id, this);
    }

    if (g_DrawGrass) {
        static float time = 0.0f;
        m_GrassShader->bind();
        // Grass VS Uniforms
        m_GrassShader->set_matrix4fv("u_ViewMatrix", &camera.get_view_matrix(true)[0][0]);
        m_GrassShader->set_matrix4fv("u_ProjectionMatrix", &camera.get_projection_matrix()[0][0]);
        glm::vec3 camera_pos = camera.get_position();
        m_GrassShader->set_float3v("u_CameraPosition", 1, &camera.get_position()[0]);
        m_GrassShader->set_float3("u_SystemBoundsMin", bbox_min.x, 0, bbox_min.z);
        m_GrassShader->set_float3("u_SystemBoundsMax", bbox_max.x, 0, bbox_max.z);
        m_GrassShader->set_int3("u_ParticlesPerDim", grass_per_dim.x, 1, grass_per_dim.y);
        m_GrassShader->set_float("u_Time", time);
        m_GrassShader->set_float("u_WindAmp", u_WindAmp);
        m_GrassShader->set_float("u_WindFactor", u_WindFactor);
        m_GrassShader->set_float2("u_WindDirection", u_WindDirection.x, u_WindDirection.y);
        m_GrassShader->set_int("u_WindTexture", 0);
        m_GrassShader->set_int("u_VerticesPerBlade", vertices_per_blade);
        noise_marble_tex.bind(0);

        // Grass FS Uniforms
        m_GrassShader->set_float3("u_LightDirection", directional_light.x, directional_light.y, directional_light.z);
        m_GrassShader->set_matrix4fv("u_LightViewProjection", &environment_settings.light_view_projection[0][0]);
        m_GrassShader->set_int("u_ShadowMap", 2);
        GL_CHECK(glActiveTexture(GL_TEXTURE2));
        GL_CHECK(glBindTexture(GL_TEXTURE_2D, m_ShadowMapBuffer->get_depth_attachment()));

        grass_system.draw_instanced(vertices_per_blade);
        m_GrassShader->unbind();
    }

    /* RENDER TO DEFAULT FRAMEBUFFER + PRESENT */
    {
        m_DefaultFrameBuffer->unbind();
        GLuint rendered_texture = m_DefaultFrameBuffer->get_color_attachment(0);
        if (draw_shadow_map)
            rendered_texture = m_VarianceMapBuffer->get_color_attachment(0);
        if (draw_depthbuffer)
            rendered_texture = m_DefaultFrameBuffer->get_depth_attachment();
        GL_CHECK(glClearColor(0.0, 0.0, 1.0, 1.0));
        GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
        GL_CHECK(glDisable(GL_DEPTH_TEST));

        static GLuint empty_vao = 0;
        if (!empty_vao)
            GL_CHECK(glGenVertexArrays(1, &empty_vao));
        GL_CHECK(glBindVertexArray(empty_vao));
        m_FramebufferShader->bind();
        m_FramebufferShader->set_int("u_Texture", 0);
        GL_CHECK(glActiveTexture(GL_TEXTURE0));
        GL_CHECK(glBindTexture(GL_TEXTURE_2D, rendered_texture));
        m_FramebufferShader->set_int("u_DrawDepth", (int)draw_depthbuffer);
        GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, 6));
        GL_CHECK(glBindVertexArray(0));
        m_FramebufferShader->unbind();
        GL_CHECK(glBindTexture(GL_TEXTURE_2D, 0));
    }
}

void Scene::DrawGUI(const Camera& camera)
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

    ImGui::Dummy(ImVec2(0.0, 5.0));
    if (ImGui::CollapsingHeader("ImGuizmo"))
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

        bool object_selected = m_ActiveEntity.GetID() != Entity::Invalid().GetID();
        if (object_selected)
        {
            ImGui::Text("Selected model id: %d", m_ActiveEntity.GetID());
            TransformComponent& tc = m_ActiveEntity.GetComponent<TransformComponent>();
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
        ImGui::SliderFloat2("Wind direction", (float*)&u_WindDirection, -1.0, 1.0);
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

    ImGui::End(); // Settings panel

    ImGui::Begin("Hierarchy");
    m_EntityRegistry.view<NameComponent>().each(
        [&](auto entity, auto namecomp) {
            ImGui::PushItemWidth(-1.0f);
            if (ImGui::Button(namecomp.name.c_str(), ImVec2(ImGui::GetWindowSize().x, 0.0f)))
                m_ActiveEntity = Entity(entity, this);
            ImGui::PopItemWidth();
        }
    );
    ImGui::End(); // Hierarchy

    ImGui::Begin("Inspector");
    if (m_ActiveEntity.GetID() != Entity::Invalid().GetID())
    {
        ImGui::Text("Id: %d", m_ActiveEntity.GetID());
        DrawComponentUIIfExists<NameComponent>(m_ActiveEntity);
        DrawComponentUIIfExists<TransformComponent>(m_ActiveEntity);
        DrawComponentUIIfExists<ModelRendererComponent>(m_ActiveEntity);
        DrawComponentUIIfExists<QuadRendererComponent>(m_ActiveEntity);
        DrawComponentUIIfExists<MaterialComponent>(m_ActiveEntity);
    }
    ImGui::End(); // Inspector

    ImGui::Render();
#ifdef __APPLE__
    ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), pCmdBuffer, pEnc);
#else
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif
}

template <typename Component>
void Scene::DrawComponentUIIfExists(Entity entity)
{
    if (entity.HasComponents<Component>())
    {
        Component::DrawUI(entity.GetComponent<Component>());
    }
}

Entity Scene::CreateEntity(const std::string& name)
{
	Entity entity(m_EntityRegistry.create(), this);
	m_EntityRegistry.emplace<NameComponent>(entity, name);
	m_EntityRegistry.emplace<TransformComponent>(entity);
	return entity;
}

void Scene::DestroyEntity(Entity entity)
{
	m_EntityRegistry.destroy((entt::entity) entity.GetID());
}




