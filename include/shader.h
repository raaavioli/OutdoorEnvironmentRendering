#pragma once
#include<map>
#include<iostream>
#include<fstream>
#include<string>

#include <glad/glad.h>

#include "assets.h"

class Shader {
public:

	Shader(const char* file_name);

	inline void bind() { glUseProgram(renderer_id); }
	inline void unbind() { glUseProgram(0); }

	void set_int(const std::string&, const int);
	void set_int3(const std::string&, const int, const int, const int);
	void set_float(const std::string&, const float);
	void set_float2(const std::string&, const float, const float);
	void set_float3(const std::string&, const float, const float, const float);
	void set_float4(const std::string&, const float, const float, const float, const float);
	void set_float3v(const std::string&, size_t, const float*);
	void set_matrix4fv(const std::string&, const float*);

private:
	GLuint gl_get_shader_type(const std::string& shader_type_str);
	std::string gl_get_shader_type_str(GLuint shader_type);

	std::map<GLuint, std::string> read_shader_file(const std::string& file_path);
	void create_program(const std::map<GLuint, std::string> shader_sources);

	void find_uniform_location_if_not_exists(const char* name);

private:
	GLuint renderer_id = 0;
	std::map<std::string, GLuint> uniform_locations;
};