#include "particlesystem.h"

#include <iostream>

ParticleSystem::ParticleSystem(int n) : num_particles(n) 
{
	particles.resize(n);
	int dim = pow(particles.size(), 1 / 3.f);
	for (int i = 0; i < particles.size(); i++)
	{
		float x = (i % dim) / (float) dim;
		float y	= ((i / dim) % dim) / (float) dim;
		float z = i / (dim * dim) / (float) dim;
		particles[i].position = { 10 * (-1/2.0 + x), 10 * (-1/2.0 + y), 10 * (-1 + z) };
		particles[i].velocity = { 0, 0, 0 };
		particles[i].size = { 0.01, 0.01 };
	}

	glGenVertexArrays(1, &this->renderer_id);
	glBindVertexArray(this->renderer_id);
	glGenBuffers(1, &this->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Particle) * particles.size(), particles.data(), GL_DYNAMIC_DRAW);

	// Bind buffers to VAO
	glBindVertexArray(this->renderer_id);
	glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
	glEnableVertexAttribArray(0); // Position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (const void*)offsetof(Particle, position));
	glEnableVertexAttribArray(1); // Size
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Particle), (const void*)offsetof(Particle, size));
	glBindVertexArray(0);
}

void ParticleSystem::draw() 
{
	glBindVertexArray(this->renderer_id);
	glDrawArrays(GL_POINTS, 0, num_particles);
	glBindVertexArray(0);
}