#pragma once
#include<map>
#include<iostream>
#include<fstream>
#include<string>

#include <glad/glad.h>

#include "assets.h"

//#define MESH_SHADER_SUPPORT

class Shader {
	friend class ShaderManager;

public:
	inline void bind() { glUseProgram(renderer_id); }
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

private:
	Shader(const char* file_name); // Created in ShaderManager

	void init();
	void reload();
	void read_shader_file(const std::string& file_path, std::map<GLuint, std::string>& output_sources);

	GLuint gl_get_shader_type(const std::string& shader_type_str);
	std::string gl_get_shader_type_str(GLuint shader_type);

	void find_uniform_location_if_not_exists(const char* name);

private:
	Shader() = delete;

	GLuint renderer_id = 0;
	const std::string m_file_name;
	std::map<std::string, GLuint> uniform_locations;
};


class ShaderManager
{
public:
    static ShaderManager& Get()
    {
        static ShaderManager instance;
        return instance;
    }

	static Shader* Create(const char* file_name);

	/*
	* Reloads all shaders from source avaliable in /../build/assets/shaders
	* 
	* NOTE: Currently the project has to be rebuilt manually (by running cmake) for the shaders to 
	* be loaded into /../build/assets/shaders from the source asset folder.
	*/
	static void Reload();

private:
    ShaderManager() {}

	std::map<std::string, Shader*> m_shaders;

public:
	ShaderManager(ShaderManager const&) = delete;
    void operator=(ShaderManager const&) = delete;
};
