#include "model.h"

#include <cassert>

RawModel::RawModel(const std::vector<Vertex>& data, const std::vector<uint32_t>& indices, GLenum usage) : m_Usage(usage) {
    assert((indices.size() % 3) == 0);
    int vertex_size = sizeof(Vertex);

    // TODO: Potentially fix usage for vertex and index buffers so they don't have to be the same.
    glGenVertexArrays(1, &m_VAO);
    glBindVertexArray(m_VAO);
    glGenBuffers(1, &m_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, data.size() * vertex_size, &data[0], usage);
    glGenBuffers(1, &m_EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), &indices[0], usage);
    m_IndexCount = indices.size();

    // Bind buffers to VAO
    bind();
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glEnableVertexAttribArray(0); // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertex_size, (const void*) offsetof(Vertex, position));
    glEnableVertexAttribArray(1); // Normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vertex_size, (const void*) offsetof(Vertex, normal));
    glEnableVertexAttribArray(2); // UV
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, vertex_size, (const void*) offsetof(Vertex, uv));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    unbind();
};

RawModel::RawModel(const ModelData& model_data) : m_Usage(GL_STATIC_DRAW)
{
    glGenVertexArrays(1, &m_VAO);
    glBindVertexArray(m_VAO);
    glGenBuffers(1, &m_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, model_data.vertices.size() * sizeof(Vertex), &model_data.vertices[0], m_Usage);
    glGenBuffers(1, &m_EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, model_data.indices.size() * sizeof(uint32_t), &model_data.indices[0], m_Usage);
    this->m_IndexCount = model_data.indices.size();

    // Bind buffers to VAO
    bind();
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glEnableVertexAttribArray(0); // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(1); // Normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2); // UV
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, uv));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    unbind();
}

RawModel::~RawModel()
{
    glDeleteBuffers(1, &m_VBO);
    glDeleteBuffers(1, &m_EBO);
    glDeleteVertexArrays(1, &m_VAO);
}

void RawModel::update_vertex_data(const std::vector<Vertex>& vertices) {
    if (this->m_Usage == GL_DYNAMIC_DRAW || this->m_Usage == GL_STREAM_DRAW) {
        this->bind();
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(Vertex), &vertices[0]);
        this->unbind();
    } else {
        std::cout << "ERROR (update_data): Usage of model data has to be GL_DYNAMIC_DRAW or GL_STREAM_DRAW" << std::endl;
    }
}

void RawModel::update_index_data(const std::vector<uint32_t>& indices) {
    if (this->m_Usage == GL_DYNAMIC_DRAW || this->m_Usage == GL_STREAM_DRAW) {
        this->bind();
        m_IndexCount = indices.size();
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(uint32_t), &indices[0]);
        this->unbind();
    } else {
        std::cout << "ERROR (update_data): Usage of model data has to be GL_DYNAMIC_DRAW or GL_STREAM_DRAW" << std::endl;
    }
}


Skybox::Skybox(TextureCubeMap* cube_map) : m_CubeMapTexture(cube_map){
    init();
}

void Skybox::draw() {
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES, this->index_count, GL_UNSIGNED_INT, 0);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
}

void Skybox::init() {
    std::vector<float> vertices = {
        -0.5f, -0.5f, -0.5f,
        0.5f, -0.5f, -0.5f,
        0.5f, 0.5f, -0.5f,
        -0.5f, 0.5f, -0.5f,
        -0.5f, -0.5f, 0.5f,
        0.5f, -0.5f, 0.5f,
        0.5f, 0.5f, 0.5f,
        -0.5f, 0.5f, 0.5f,
    };

    std::vector<uint32_t> indices = {
        0, 1, 2, // Back
        2, 3, 0, 
        3, 7, 4, // Left
        4, 0, 3,
        4, 7, 6, // Front
        6, 5, 4,
        6, 2, 1, // Right
        1, 5, 6,
        2, 6, 7, // Top
        7, 3, 2,
        0, 4, 5, // Bottom
        5, 1, 0,
    };

    glGenVertexArrays(1, &m_VAO);
    glBindVertexArray(m_VAO);
    glGenBuffers(1, &m_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
    glGenBuffers(1, &m_EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), &indices[0], GL_STATIC_DRAW);

    // Bind buffers to VAO
    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glEnableVertexAttribArray(0); // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (const void*) 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBindVertexArray(0);
}