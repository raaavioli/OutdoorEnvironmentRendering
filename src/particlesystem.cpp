#include "particlesystem.h"

#include <iostream>
#include <random>
#include <glm/gtx/norm.hpp>

ParticleSystem::ParticleSystem(int n) : num_particles(n) 
{
	particles.resize(n);
	int dim = pow(particles.size(), 1 / 3.f);
	for (int i = 0; i < particles.size(); i++)
	{
		float x = (i % dim) / (float) dim;
		float y	= ((i / dim) % dim) / (float) dim;
		float z = i / (dim * dim) / (float) dim;
		particles[i].position = 5.0f * glm::vec3((-1/2.0 + x), (-1/2.0 + y), (-1 + z));
		particles[i].velocity = { 0, 0, 0 };
		particles[i].size = { 0.01, 0.01 };
		particles[i].cluster = 0.0f;
	}
	approx_cluster_size = 128;
	num_clusters = particles.size() / approx_cluster_size;

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

void ParticleSystem::clusterize() 
{
	std::default_random_engine generator;
	generator.seed(0);

	// K-means++ initialization
	// 1. Choose one center uniformly at random among the data points.
	int center_index = particles.size() * (generator() / (float)(generator.max() - generator.min()));
	std::vector<glm::vec3> centers;
	centers.reserve(num_clusters);
	centers.push_back(particles[center_index].position);

	std::vector<float> distancesSq;
	distancesSq.reserve(particles.size());

	// 4. Repeat Steps 2 and 3 until k centers have been chosen.
	for (int k = 0; k < num_clusters - 1; k++) {
		// 2. For each data point x not chosen yet, compute D(x), the distance between x and the nearest center that has already been chosen.
		for (int i = 0; i < particles.size(); i++)
		{
			glm::vec3 current_particle = particles[i].position;
			glm::vec3 closest_center = centers[particles[i].cluster];
			float closest_distance = glm::length2(closest_center - current_particle);
			
			glm::vec3 newest_center = centers[centers.size() - 1];
			float newest_distance = glm::length2(newest_center - current_particle);
			if (newest_distance < closest_distance) {
				closest_center = newest_center;
				closest_distance = newest_distance;
				particles[i].cluster = centers.size() - 1;
			}
			distancesSq.push_back(closest_distance);
		}


		// 3. Choose one new data point at random as a new center, using a weighted probability distribution where a point x is chosen with probability proportional to D(x)^2.
		std::discrete_distribution<int> distance_distribution (distancesSq.begin(), distancesSq.end());
		center_index = distance_distribution(generator);
		centers.push_back(particles[center_index].position);
		distancesSq.clear();
	}

	glBindVertexArray(this->renderer_id);
	int size = sizeof(Particle) * particles.size();
	void* data = &particles[0];

	glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
	void* ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	memcpy(ptr, data, size);
	glUnmapBuffer(GL_ARRAY_BUFFER);

	glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
}

void ParticleSystem::draw_clusters()
{
	static bool clustered = false;
	if (!clustered) {
		clusterize();
		clustered = true;
	}

	glBindVertexArray(this->renderer_id);
	glDrawArrays(GL_POINTS, 0, num_particles);
	glBindVertexArray(0);
}