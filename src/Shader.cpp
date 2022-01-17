#include "shader.h"

#include<vector>

#include<gl_helpers.h>

Shader::Shader(const char* file_name)
{
  std::map<GLuint, std::string> shader_sources = read_shader_file(Assets::shaders_path + std::string(file_name));
  create_program(shader_sources);
}

/**
* Get GL shader type from string
*/
GLuint Shader::gl_get_shader_type(const std::string& shader_type_str)
{
    if (shader_type_str.compare("FRAGMENT") == 0)
        return GL_FRAGMENT_SHADER;
    else if (shader_type_str.compare("VERTEX") == 0)
        return GL_VERTEX_SHADER;
    else if (shader_type_str.compare("GEOMETRY") == 0)
        return GL_GEOMETRY_SHADER;
    else if (shader_type_str.compare("COMPUTE") == 0)
        return GL_COMPUTE_SHADER;
    else if (shader_type_str.compare("MESH") == 0)
        return GL_MESH_SHADER_NV;
    else if (shader_type_str.compare("TASK") == 0)
        return GL_TASK_SHADER_NV;

    return GL_INVALID_VALUE;
}

/**
* Get GL shader string from type
*/
std::string Shader::gl_get_shader_type_str(GLuint shader_type)
{
    if (shader_type == GL_FRAGMENT_SHADER)
        return std::string("FRAGMENT");
    else if (shader_type == GL_VERTEX_SHADER)
        return std::string("VERTEX");
    else if (shader_type == GL_GEOMETRY_SHADER)
        return std::string("GEOMETRY");
    else if (shader_type == GL_COMPUTE_SHADER)
        return std::string("COMPUTE");
    else if (shader_type == GL_MESH_SHADER_NV)
        return std::string("MESH");
    else if (shader_type == GL_TASK_SHADER_NV)
        return std::string("TASK");

    return "_INVALID_TYPE_";
}

void Shader::find_uniform_location_if_not_exists(const char* name)
{
  if (uniform_locations.find(name) == uniform_locations.end())
  {
    GLuint location = glGetUniformLocation(renderer_id, name);
    uniform_locations[name] = location;
  }
}

void Shader::set_int(const std::string& name, const int value)
{
  find_uniform_location_if_not_exists(name.c_str());
  GL_CHECK(glUniform1i(uniform_locations[name], value));
}

void Shader::set_int3(const std::string& name, const int v1, const int v2, const int v3)
{
  find_uniform_location_if_not_exists(name.c_str());
  GL_CHECK(glUniform3i(uniform_locations[name], v1, v2, v3));
}

void Shader::set_float(const std::string& name, const float value)
{
  find_uniform_location_if_not_exists(name.c_str());
  GL_CHECK(glUniform1f(uniform_locations[name], value));
}

void Shader::set_float3(const std::string& name, const float v1, const float v2, const float v3)
{
  find_uniform_location_if_not_exists(name.c_str());
  GL_CHECK(glUniform3f(uniform_locations[name], v1, v2, v3));
}

void Shader::set_float4(const std::string& name, const float v1, const float v2, const float v3, const float v4)
{
  find_uniform_location_if_not_exists(name.c_str());
  GL_CHECK(glUniform4f(uniform_locations[name], v1, v2, v3, v4));
}

void Shader::set_float3v(const std::string& name, size_t count, const float* values) 
{
  find_uniform_location_if_not_exists(name.c_str());
  GL_CHECK(glUniform3fv(uniform_locations[name], count, values));
}

void Shader::set_matrix4fv(const std::string& name, const float* value_ptr)
{
  find_uniform_location_if_not_exists(name.c_str());
  GL_CHECK(glUniformMatrix4fv(uniform_locations[name], 1, false, value_ptr));
}

void Shader::create_program(const std::map<GLuint, std::string> shader_sources)
{
  if (renderer_id)
    return;
  renderer_id = glCreateProgram();

  int success;
  char info_log[512];
  std::vector<GLuint> shader_ids;
  for (auto& [shader_type, shader_code] : shader_sources)
  {
    GLuint shader_id = glCreateShader(shader_type);
    const char* shader_code_cstr = shader_code.c_str();
    glShaderSource(shader_id, 1, &shader_code_cstr, NULL);
    glCompileShader(shader_id);
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);
    if (!success) {
      glGetShaderInfoLog(shader_id, sizeof(info_log), NULL, info_log);
      std::cout << "ERROR: "<< gl_get_shader_type_str(shader_type) << " shader compilation failed\n" << info_log << std::endl;
    };
    glAttachShader(renderer_id, shader_id);
    shader_ids.push_back(shader_id);
  }

  glLinkProgram(renderer_id);
  glGetProgramiv(renderer_id, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(renderer_id, sizeof(info_log), NULL, info_log);
    std::cout << "ERROR: Shader program linkage failed\n" << info_log << std::endl;
  }

  for (GLuint id : shader_ids)
    glDeleteShader(id);
}

std::map<GLuint, std::string> Shader::read_shader_file(const std::string& file_path)
{
  std::string shader_source = Assets::read_file(file_path);
  if (shader_source.size() == 0)
    return std::map<GLuint, std::string>();

  std::map<GLuint, std::string> shader_map;

  size_t pos = 0;
  const std::string TYPE_START = "__";
  // TODO: Currently windows only using \r\n. Make OS agnostic
  const std::string CARRIAGE_RETURN = "\r\n";
  const std::string TYPE_END = TYPE_START + CARRIAGE_RETURN;
  while (pos != std::string::npos)
  {
    pos = shader_source.find(TYPE_START, pos);
    if (pos == std::string::npos)
    {
      std::cout << "Error: Could not find shader type in '" << file_path << "'" << std::endl;
      break;
    }
    pos += TYPE_START.size();

    size_t type_end = shader_source.find(TYPE_END, pos);
    std::string shader_type = shader_source.substr(pos, type_end - pos);
    GLuint gl_shader_type = gl_get_shader_type(shader_type);
    if (gl_shader_type == GL_INVALID_VALUE)
    {
      std::cout << "Error: Shader type '" << shader_type << "' in '" << file_path << "' is incorrect" << std::endl;
      break;
    }
    pos = type_end + TYPE_END.size();

    // Find start of next shader or end of file
    size_t next_pos = shader_source.find(TYPE_START, pos);
    std::string shader_code;
    if (next_pos == std::string::npos)
      shader_code = shader_source.substr(pos);
    else
      shader_code = shader_source.substr(pos, next_pos - pos);

    shader_map.emplace(std::make_pair(gl_shader_type, shader_code));

    pos = next_pos;
  }
  return shader_map;
}