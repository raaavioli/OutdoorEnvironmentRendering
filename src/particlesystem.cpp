#include "particlesystem.h"

#include <iostream>
#include <limits>

#include <glm/gtx/norm.hpp>

#include "clock.h"
#include "gl_helpers.h"

ParticleSystem::ParticleSystem(glm::ivec3 particles_per_dim, glm::vec3 bbox_min, glm::vec3 bbox_max, GLuint simulation_shader)
	: particles_per_dim(particles_per_dim), bbox_min(bbox_min), bbox_max(bbox_max), simulation_shader(simulation_shader)
{
	particles.resize(particles_per_dim.x * particles_per_dim.y * particles_per_dim.z);
	glm::ivec3 clusters_per_dim = (particles_per_dim + particles_per_cluster_dim - 1) / particles_per_cluster_dim;
	num_clusters = clusters_per_dim.x * clusters_per_dim.y * clusters_per_dim.z;

	for (int z = 0; z < particles_per_dim.z; z++)
	{
		for (int y = 0; y < particles_per_dim.y; y++)
		{
			for (int x = 0; x < particles_per_dim.x; x++)
			{
				int i = particles_per_dim.x * (z * particles_per_dim.y + y) + x;
				particles[i].position = bbox_min + (bbox_max - bbox_min) * (glm::vec3(x, y, z) / (glm::vec3)particles_per_dim);
				particles[i].velocity = { 0, -2.f, 0 };
				particles[i].size = { 0.05f, 0.05f };
			}
		}
	}

	/* SHADER STORAGE BUFFER BINDINGS */
	GL_CHECK(glGenVertexArrays(1, &vao));

	GL_CHECK(glGenBuffers(1, &ssbo));
	GL_CHECK(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo));
	GL_CHECK(glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Particle) * particles.size(), &particles[0], GL_DYNAMIC_DRAW));

	GL_CHECK(glBindVertexArray(vao));
	GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, ssbo));
	GL_CHECK(glEnableVertexAttribArray(0)); // Position
	GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (const void*)offsetof(Particle, position)));
	GL_CHECK(glEnableVertexAttribArray(1)); // Velocity
	GL_CHECK(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (const void*)offsetof(Particle, velocity)));
	GL_CHECK(glEnableVertexAttribArray(2)); // Size
	GL_CHECK(glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Particle), (const void*)offsetof(Particle, size)));
	GL_CHECK(glBindVertexArray(0));

	u_time_delta = glGetUniformLocation(simulation_shader, "u_time_delta");
	u_time = glGetUniformLocation(simulation_shader, "u_time");

	u_num_particles = glGetUniformLocation(simulation_shader, "u_num_particles");
	u_bboxmin = glGetUniformLocation(simulation_shader, "u_BboxMin");
	u_bboxmax = glGetUniformLocation(simulation_shader, "u_BboxMax");
	u_particles_per_dim = glGetUniformLocation(simulation_shader, "u_ParticlesPerDim");
	
	GL_CHECK(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0));
}

int ParticleSystem::get_cluster(glm::vec3 position)
{
	glm::ivec3 relative_pos = ((glm::vec3)particles_per_dim) * (position - bbox_min) / (bbox_max - bbox_min);
	relative_pos = glm::min(relative_pos, particles_per_dim - 1);
	relative_pos = relative_pos / particles_per_cluster_dim;

	glm::ivec3 clusters_per_dim = (particles_per_dim + particles_per_cluster_dim - 1) / particles_per_cluster_dim;

	int cluster = clusters_per_dim.x * (relative_pos.z * clusters_per_dim.y + relative_pos.y) + relative_pos.x;

	assert(cluster < num_clusters);
	return cluster;
}

void ParticleSystem::draw()
{
	GL_CHECK(glBindVertexArray(vao));
	GL_CHECK(glDrawArrays(GL_POINTS, 0, particles.size()));
	GL_CHECK(glBindVertexArray(0));
}

void ParticleSystem::update(float dt)
{
	static float time = 0.0f;
	time += dt;

	/* COMPUTE UPDATE */
	GL_CHECK(glUseProgram(simulation_shader));
	GL_CHECK(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo));
	GL_CHECK(glUniform1f(u_time_delta, dt));
	GL_CHECK(glUniform1f(u_time, time));
	GL_CHECK(glUniform1i(u_num_particles, particles.size()));
	GL_CHECK(glUniform3f(u_bboxmin, bbox_min.x, bbox_min.y, bbox_min.z));
	GL_CHECK(glUniform3f(u_bboxmax, bbox_max.x, bbox_max.y, bbox_max.z));
	GL_CHECK(glUniform3i(u_particles_per_dim, particles_per_dim.x, particles_per_dim.y, particles_per_dim.z));
	GL_CHECK(glDispatchCompute(particles.size(), 1, 1));

	GL_CHECK(glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT));
	GL_CHECK(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0));
	GL_CHECK(glUseProgram(0));
}