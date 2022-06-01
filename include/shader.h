#pragma once
#include<map>
#include<iostream>
#include<fstream>
#include<string>

#include <glad/glad.h>

//#define MESH_SHADER_SUPPORT

class Shader {
	friend class ShaderManager;
	friend class AssetManager;

public:
	inline void bind() { glUseProgram(m_Handle); }
	inline void unbind() { glUseProgram(0); }

	void set_uint(const std::string& name, const uint32_t value);
	void set_int(const std::string&, const int);
	void set_int3(const std::string&, const int, const int, const int);
	void set_float(const std::string&, const float);
	void set_float2(const std::string&, const float, const float);
	void set_float3(const std::string&, const float, const float, const float);
	void set_float4(const std::string&, const float, const float, const float, const float);
	void set_float3v(const std::string&, size_t, const float*);
	void set_matrix4fv(const std::string&, const float*);

	static GLuint GetGLShaderTypeFromString(const std::string& shader_type_str);
	static std::string GetStringFromGLShaderType(GLuint shader_type);

private:
	Shader(const std::string& filename, const std::map<GLuint, std::string>& shader_sources); // Created in ShaderManager

	void Init(const std::map<GLuint, std::string>& shader_sources);
	void Reload(const std::map<GLuint, std::string>& shader_sources);

	void FindUniformLocationIfNotExists(const char* name);

private:
	Shader() = delete;

	GLuint m_Handle = 0;
	std::string m_FileName;
	std::map<std::string, GLuint> m_UniformLocations;
};

