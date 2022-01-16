#include "particlesystem.h"

#include <iostream>
#include <limits>

#include <glm/gtx/norm.hpp>

#include "clock.h"
#include "gl_helpers.h"

ParticleSystem::ParticleSystem(glm::ivec3 particles_per_dim, glm::vec3 bbox_min, glm::vec3 bbox_max)
	: particles_per_dim(particles_per_dim), bbox_min(bbox_min), bbox_max(bbox_max)
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
				particles[i].velocity = { 0, -0.5f, 0 };
				particles[i].width = 0.05f;
				particles[i].height = 0.05f;
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
	GL_CHECK(glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), (const void*)offsetof(Particle, width)));
	GL_CHECK(glEnableVertexAttribArray(3)); // Size
	GL_CHECK(glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), (const void*)offsetof(Particle, height)));
	GL_CHECK(glBindVertexArray(0));
	
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

void ParticleSystem::update(float dt, Shader& particle_cs)
{
	static float time = 0.0f;
	time += dt;

	/* COMPUTE UPDATE */
	particle_cs.bind();
	particle_cs.set_float("u_time_delta", dt);
	particle_cs.set_float("u_time", time);
	particle_cs.set_int("u_num_particles", particles.size());
	particle_cs.set_float3("u_BboxMin", bbox_min.x, bbox_min.y, bbox_min.z);
	particle_cs.set_float3("u_BboxMax", bbox_max.x, bbox_max.y, bbox_max.z);
	particle_cs.set_int3("u_ParticlesPerDim", particles_per_dim.x, particles_per_dim.y, particles_per_dim.z);

	GL_CHECK(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo));
	GL_CHECK(glDispatchCompute((particles.size() + 1024 - 1) / 1024, 1, 1));

	GL_CHECK(glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT));
	GL_CHECK(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0));
	particle_cs.unbind();
}