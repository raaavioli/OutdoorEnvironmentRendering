#ifndef WR_MODEL_H
#define WR_MODEL_H

#include <vector>
#include <string>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "texture.h"
#include "material.h"

struct Vertex 
{
    glm::vec3 position;
    glm::vec4 color;
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
    RawModel(const char* filename);
    ~RawModel() { glDeleteVertexArrays(1, &renderer_id); };

    void update_vertex_data(const std::vector<Vertex>& vertices);
    void update_index_data(const std::vector<uint32_t>& indices);

    inline void bind() { glBindVertexArray(renderer_id); }
    inline void unbind() { glBindVertexArray(0); }
    inline void draw() { glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, 0); }

private:
    bool load_fbx(ModelData& model_data, const std::string& file_path);

private:
    GLuint renderer_id, vbo, ebo;
    GLenum gl_usage;
    uint32_t index_count;
};

struct Skybox {
    /**
     * Create a sky box from a file path or name of cube map image(s).
     * 
     * @param filename file name of sky box texture, image has to have 4:3 width:height ratio..
     */
    Skybox(const char* filename, bool folder = false);

    /**
     * Draw a skybox, assumes appropriate shader is used
     */
    void draw();

    inline void bind_cube_map(uint32_t slot) const { cube_map.bind(slot); };
    inline void unbind_cube_map() const { cube_map.unbind(); };

private:
    TextureCubeMap cube_map;
    GLuint renderer_id, vbo, ebo;
    const uint32_t index_count = 36;

    void init();
};

#endif //WR_MODEL_H