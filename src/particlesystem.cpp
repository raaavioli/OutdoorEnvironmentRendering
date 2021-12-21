#include "particlesystem.h"

#include <iostream>
#include <limits>

#include <glm/gtx/norm.hpp>

#include "clock.h"

ParticleSystem::ParticleSystem(glm::ivec3 particles_per_dim, glm::vec3 bbox_min, glm::vec3 bbox_max)
	: particles_per_dim(particles_per_dim), bbox_min(bbox_min), bbox_max(bbox_max)
{
	particles.resize(particles_per_dim.x * particles_per_dim.y * particles_per_dim.z);
	glm::ivec3 clusters_per_dim = (particles_per_dim + particles_per_cluster_dim - 1) / particles_per_cluster_dim;
	num_clusters = clusters_per_dim.x * clusters_per_dim.y * clusters_per_dim.z;
	cluster_counts = std::vector(num_clusters, 0);

	for (int z = 0; z < particles_per_dim.z; z++)
	{
		for (int y = 0; y < particles_per_dim.y; y++)
		{
			for (int x = 0; x < particles_per_dim.x; x++)
			{
				int i = particles_per_dim.x * (z * particles_per_dim.y + y) + x;
				particles[i].position = bbox_min + (bbox_max - bbox_min) * (glm::vec3(x, y, z) / (glm::vec3)particles_per_dim);
				particles[i].velocity = { 0, -1, 0 };
				particles[i].size = { 0.1, 0.1 };
				int cluster = get_cluster(particles[i].position);
				particles[i].cluster = cluster;
				cluster_counts[cluster]++;
			}
		}
	}

	glGenVertexArrays(1, &this->renderer_id);
	glBindVertexArray(this->renderer_id);
	glGenBuffers(1, &this->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Particle) * particles.size(), &particles[0], GL_DYNAMIC_DRAW);

	// Bind buffers to VAO
	glBindVertexArray(this->renderer_id);
	glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
	glEnableVertexAttribArray(0); // Position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (const void*)offsetof(Particle, position));
	glEnableVertexAttribArray(1); // Size
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Particle), (const void*)offsetof(Particle, size));
	glEnableVertexAttribArray(2); // Cluster
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), (const void*)offsetof(Particle, cluster));
	glBindVertexArray(0);
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
	glBindVertexArray(this->renderer_id);

	glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Particle) * particles.size(), &particles[0]);

	glDrawArrays(GL_POINTS, 0, particles.size());
	glBindVertexArray(0);
}

void ParticleSystem::update(float dt)
{
	static float time = 0.0f;
	time += dt;

	for (auto& particle : particles)
	{
		particle.position += dt * particle.velocity;
		if (particle.position.y < bbox_min.y) {
			particle.position.y = bbox_max.y;
		}
	}
}