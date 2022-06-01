#ifndef WR_MODEL_H
#define WR_MODEL_H

#include <vector>
#include <string>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "texture.h"

struct Vertex 
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

struct ModelData
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

struct RawModel {
public:
    RawModel(const std::vector<Vertex>& data, const std::vector<uint32_t>& indices, GLenum usage);
    RawModel(const ModelData& model_data);
    ~RawModel();

    void update_vertex_data(const std::vector<Vertex>& vertices);
    void update_index_data(const std::vector<uint32_t>& indices);

    inline void bind() { glBindVertexArray(m_VAO); }
    inline void unbind() { glBindVertexArray(0); }
    inline void draw() { glDrawElements(GL_TRIANGLES, m_IndexCount, GL_UNSIGNED_INT, 0); }

private:

private:
    GLuint m_VAO, m_VBO, m_EBO;
    GLenum m_Usage;
    uint32_t m_IndexCount;
};

struct Skybox {
    /**
     * Create a sky box from a file path or name of cube map image(s).
     * 
     * @param filename file name of sky box texture, image has to have 4:3 width:height ratio..
     */
    Skybox(TextureCubeMap* cube_map);

    /**
     * Draw a skybox, assumes appropriate shader is used
     */
    void draw();

    inline void bind_cube_map(uint32_t slot) const { m_CubeMapTexture->bind(slot); };
    inline void unbind_cube_map() const { m_CubeMapTexture->unbind(); };

private:
    TextureCubeMap* m_CubeMapTexture;
    GLuint m_VAO, m_VBO, m_EBO;
    const uint32_t index_count = 36;

    void init();
};

#endif //WR_MODEL_H