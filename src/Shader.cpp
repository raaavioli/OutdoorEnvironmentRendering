#include "shader.h"

#include<vector>

#include<gl_helpers.h>

Shader::Shader(const std::string& filename, const std::map<GLuint, std::string>& shader_sources) : m_FileName(filename)
{
    Init(shader_sources);
}

void Shader::FindUniformLocationIfNotExists(const char* name)
{
    if (m_UniformLocations.find(name) == m_UniformLocations.end())
    {
        GLuint location = glGetUniformLocation(m_Handle, name);
        m_UniformLocations[name] = location;
    }
}

void Shader::set_uint(const std::string& name, const uint32_t value)
{
    FindUniformLocationIfNotExists(name.c_str());
    GL_CHECK(glUniform1ui(m_UniformLocations[name], value));
}

void Shader::set_int(const std::string& name, const int value)
{
    FindUniformLocationIfNotExists(name.c_str());
    GL_CHECK(glUniform1i(m_UniformLocations[name], value));
}

void Shader::set_int3(const std::string& name, const int v1, const int v2, const int v3)
{
    FindUniformLocationIfNotExists(name.c_str());
    GL_CHECK(glUniform3i(m_UniformLocations[name], v1, v2, v3));
}

void Shader::set_float(const std::string& name, const float value)
{
    FindUniformLocationIfNotExists(name.c_str());
    GL_CHECK(glUniform1f(m_UniformLocations[name], value));
}

void Shader::set_float2(const std::string& name, const float v1, const float v2)
{
    FindUniformLocationIfNotExists(name.c_str());
    GL_CHECK(glUniform2f(m_UniformLocations[name], v1, v2));
}

void Shader::set_float3(const std::string& name, const float v1, const float v2, const float v3)
{
    FindUniformLocationIfNotExists(name.c_str());
    GL_CHECK(glUniform3f(m_UniformLocations[name], v1, v2, v3));
}

void Shader::set_float4(const std::string& name, const float v1, const float v2, const float v3, const float v4)
{
    FindUniformLocationIfNotExists(name.c_str());
    GL_CHECK(glUniform4f(m_UniformLocations[name], v1, v2, v3, v4));
}

void Shader::set_float3v(const std::string& name, size_t count, const float* values) 
{
    FindUniformLocationIfNotExists(name.c_str());
    GL_CHECK(glUniform3fv(m_UniformLocations[name], count, values));
}

void Shader::set_matrix4fv(const std::string& name, const float* value_ptr)
{
    FindUniformLocationIfNotExists(name.c_str());
    GL_CHECK(glUniformMatrix4fv(m_UniformLocations[name], 1, false, value_ptr));
}

void Shader::Init(const std::map<GLuint, std::string>& shader_sources)
{
    if (m_Handle)
        return;
    m_Handle = glCreateProgram();

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
            std::cout << "ERROR: " << m_FileName << "(" << GetStringFromGLShaderType(shader_type) << ")" << " shader compilation failed\n" << info_log << std::endl;
        };
        glAttachShader(m_Handle, shader_id);
        shader_ids.push_back(shader_id);
    }

    glLinkProgram(m_Handle);
    glGetProgramiv(m_Handle, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(m_Handle, sizeof(info_log), NULL, info_log);
        std::cout << "ERROR: " << m_FileName << ". Shader program linkage failed\n" << info_log << std::endl;
    }

    for (GLuint id : shader_ids)
        glDeleteShader(id);
}

void Shader::Reload(const std::map<GLuint, std::string>& shader_sources)
{
    if (m_Handle)
    {
        glDeleteProgram(m_Handle);
        m_Handle = 0;
    }
    Init(shader_sources);

}

/**
* Get GL shader type from string
*/
GLuint Shader::GetGLShaderTypeFromString(const std::string& shader_type_str)
{
    if (shader_type_str.compare("FRAGMENT") == 0)
        return GL_FRAGMENT_SHADER;
    else if (shader_type_str.compare("VERTEX") == 0)
        return GL_VERTEX_SHADER;
    else if (shader_type_str.compare("GEOMETRY") == 0)
        return GL_GEOMETRY_SHADER;
    else if (shader_type_str.compare("COMPUTE") == 0)
        return GL_COMPUTE_SHADER;
#ifdef MESH_SHADER_SUPPORT
    else if (shader_type_str.compare("MESH") == 0)
        return GL_MESH_SHADER_NV;
    else if (shader_type_str.compare("TASK") == 0)
        return GL_TASK_SHADER_NV;
#endif

    return GL_INVALID_VALUE;
}

/**
* Get GL shader string from type
*/
std::string Shader::GetStringFromGLShaderType(GLuint gl_type)
{
    if (gl_type == GL_FRAGMENT_SHADER)
        return std::string("FRAGMENT");
    else if (gl_type == GL_VERTEX_SHADER)
        return std::string("VERTEX");
    else if (gl_type == GL_GEOMETRY_SHADER)
        return std::string("GEOMETRY");
    else if (gl_type == GL_COMPUTE_SHADER)
        return std::string("COMPUTE");
#ifdef MESH_SHADER_SUPPORT
    else if (gl_type == GL_MESH_SHADER_NV)
        return std::string("MESH");
    else if (gl_type == GL_TASK_SHADER_NV)
        return std::string("TASK");
#endif

    return "_INVALID_TYPE_";
}