#include "model.h"

#include <cassert>

#include "assets.h"
#include "OpenFBX/src/ofbx.h"

RawModel::RawModel(const std::vector<Vertex>& data, const std::vector<uint32_t>& indices, GLenum usage) : gl_usage(usage) {
    assert((indices.size() % 3) == 0);
    int vertex_size = sizeof(Vertex);

    // TODO: Potentially fix usage for vertex and index buffers so they don't have to be the same.
    glGenVertexArrays(1, &this->renderer_id);
    glBindVertexArray(this->renderer_id);
    glGenBuffers(1, &this->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * vertex_size, &data[0], usage);
    glGenBuffers(1, &this->ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), &indices[0], usage);
    this->index_count = indices.size();

    // Bind buffers to VAO
    bind();
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glEnableVertexAttribArray(0); // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertex_size, (const void*) offsetof(Vertex, position));
    glEnableVertexAttribArray(1); // Color
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, vertex_size, (const void*) offsetof(Vertex, color));
    glEnableVertexAttribArray(2); // Normal
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, vertex_size, (const void*) offsetof(Vertex, normal));
    glEnableVertexAttribArray(3); // UV
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, vertex_size, (const void*) offsetof(Vertex, uv));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ebo);
    unbind();
};

RawModel::RawModel(const char* filename)
{
  std::string file_path = Assets::models_path + std::string(filename);
  size_t ext = file_path.find_last_of(".");
  std::string ext_str = file_path.substr(ext + 1);
  if (ext_str.compare("fbx") == 0)
  {
    gl_usage = GL_STATIC_DRAW;

    ModelData fbx_model;
    if (!load_fbx(fbx_model, file_path))
      return;

    glGenVertexArrays(1, &renderer_id);
    glBindVertexArray(renderer_id);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, fbx_model.vertices.size() * sizeof(Vertex), &fbx_model.vertices[0], gl_usage);
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, fbx_model.indices.size() * sizeof(uint32_t), &fbx_model.indices[0], gl_usage);
    this->index_count = fbx_model.indices.size();
    uint32_t index_count;
  }
  else
  {
    std::cout << "Error: unsupported file extension '" << ext_str << "' for file '" << filename << "'" << std::endl;
    return;
  }
}

void RawModel::update_vertex_data(const std::vector<Vertex>& vertices) {
    if (this->gl_usage == GL_DYNAMIC_DRAW || this->gl_usage == GL_STREAM_DRAW) {
        this->bind();
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(Vertex), &vertices[0]);
        this->unbind();
    } else {
        std::cout << "ERROR (update_data): Usage of model data has to be GL_DYNAMIC_DRAW or GL_STREAM_DRAW" << std::endl;
    }
}

void RawModel::update_index_data(const std::vector<uint32_t>& indices) {
    if (this->gl_usage == GL_DYNAMIC_DRAW || this->gl_usage == GL_STREAM_DRAW) {
        this->bind();
        index_count = indices.size();
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(uint32_t), &indices[0]);
        this->unbind();
    } else {
        std::cout << "ERROR (update_data): Usage of model data has to be GL_DYNAMIC_DRAW or GL_STREAM_DRAW" << std::endl;
    }
}


bool RawModel::load_fbx(ModelData& model_data, const std::string& file_path)
{
  /*std::string file = Assets::read_file(file_path);
  ofbx::IScene* scene = ofbx::load((ofbx::u8*)file.data(), file.size(), (ofbx::u64)ofbx::LoadFlags::TRIANGULATE);
  if (!scene) {
    std::cout << ofbx::getError() << std::endl;
    return false;
  }

  // TODO: Don't assume one geometry per fbx

  const ofbx::Geometry* geom = scene->getGeometry(0);
    
  const int vertex_count = geom->getVertexCount();
  const int index_count = geom->getVertexCount();
  const ofbx::Vec3* vertices = geom->getVertices();
  const ofbx::Vec3* normals = geom->getNormals();
  const ofbx::Vec2* uvs = geom->getUVs();

  model_data.vertices.reserve(vertex_count);
  for (int v = 0; v < vertex_count; v++)
  {
    Vertex vertex;
    vertex.position = glm::vec3(vertices[v].x, vertices[v].y, vertices[v].z);
    vertex.normal = glm::vec3(normals[v].x, normals[v].y, normals[v].z);
    vertex.uv = glm::vec2(uvs[v].x, uvs[v].y);
    model_data.vertices.push_back(vertex);
  }

  model_data.indices.reserve(index_count);
  const int* face_indices = geom->getFaceIndices();
  for (int i = 0; i < index_count; i++)
  {
    int idx = (face_indices[i] < 0) ? -face_indices[i] : (face_indices[i] + 1);
    model_data.indices.push_back(idx);
  }*/
  return false;
}


Skybox::Skybox(const char* filename, bool folder) : cube_map(TextureCubeMap(filename, folder)){
    init();
}

void Skybox::draw() {
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    glBindVertexArray(this->renderer_id);
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

    glGenVertexArrays(1, &this->renderer_id);
    glBindVertexArray(this->renderer_id);
    glGenBuffers(1, &this->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
    glGenBuffers(1, &this->ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), &indices[0], GL_STATIC_DRAW);

    // Bind buffers to VAO
    glBindVertexArray(this->renderer_id);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glEnableVertexAttribArray(0); // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (const void*) 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ebo);
    glBindVertexArray(0);
}