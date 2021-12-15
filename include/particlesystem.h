#pragma once

#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

struct Particle
{
	glm::vec3 position;
	glm::vec3 velocity;
	glm::vec2 size;
};

struct ParticleSystem 
{
public:
	ParticleSystem(int num_particles);

	void draw();

private:
	int num_particles;
	std::vector<Particle> particles;
	GLuint renderer_id, vbo;
};