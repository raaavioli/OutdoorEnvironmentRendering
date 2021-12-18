#include "particlesystem.h"

#include <iostream>
#include <limits>

#include <glm/gtx/norm.hpp>

#include "clock.h"

ParticleSystem::ParticleSystem(int _num_particles, int _max_cluster_size) : num_particles(_num_particles), max_cluster_size(_max_cluster_size)
{
	generator.seed(0);
	particles.resize(num_particles);
	default_cluster.resize(num_particles);
	num_clusters = (num_particles + max_cluster_size - 1) / max_cluster_size;
	clusters.resize(num_clusters);

	int dim = pow(particles.size(), 1 / 3.f);
	for (int i = 0; i < particles.size(); i++)
	{
		float x = (i % dim) / (float) dim;
		float y	= ((i / dim) % dim) / (float) dim;
		float z = i / (dim * dim) / (float) dim;
		particles[i].id = i;
		particles[i].position = 3.f * glm::vec3((-1/2.0 + x), (-1/2.0 + y), (-1 + z));
		particles[i].velocity = { 0, 0, 0 };
		particles[i].size = { 0.02, 0.02 };
		particles[i].cluster = DEFAULT_CLUSTER;
		default_cluster[i] = &particles[i];
	}

	// Initialize cluster centroids randomly
	for (int c = 0; c < num_clusters; c++)
	{
		int random_index = particles.size() * ((generator() - generator.min()) / (float)(generator.max() - generator.min()));
		clusters[c].centroid = particles[random_index].position;
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

void ParticleSystem::draw_clusters()
{
	static bool clustered = false;
	if (!clustered) {
		rtmac_variant();
		clustered = false;
	}

	glBindVertexArray(this->renderer_id);
	glDrawArrays(GL_POINTS, 0, num_particles);
	glBindVertexArray(0);
}

void ParticleSystem::update(float dt)
{
	static float time = 0.0f;
	time += dt;
	for (int z = 0; z < 32; z++)
	{
		for (int y = 0; y < 32; y++)
		{
			for (int x = 0; x < 32; x++)
			{
				Particle& particle = particles[z * 32 * 32 + y * 32 + x];
				if (x > 16 && z > 16)
				{
					particle.position.x -= 0.04f * (sin(3 * time));
					particle.position.z -= 0.04f * (sin(3 * time));
				} 
			}
		}
	}
	//for (auto& particle : particles)
	//{
		//particle.position.x += 0.003f * sin(time + pseudo_rand(particle.id));
		//particle.position.y += 0.003f * cos(time + pseudo_rand(particle.id));
		//particle.position.z += 0.003f * sin(-time + pseudo_rand(particle.id));
	//}
}

void ParticleSystem::naive_k_means_pp()
{
	Clock clock;
	clock.prettyPrintSinceStart("Start");

	num_clusters = (particles.size() + max_cluster_size - 1) / max_cluster_size;

	// K-means++ initialization
	// 1. Choose one center uniformly at random among the data points.
	int center_index = particles.size() * ((generator() - generator.min()) / (float)(generator.max() - generator.min()));

	clusters[0].centroid = particles[center_index].position;

	std::vector<float> distancesSq;
	distancesSq.reserve(particles.size());
	clock.prettyPrintSinceStart("Init");
	// 4. Repeat Steps 2 and 3 until k centers have been chosen.
	for (int k = 1; k < num_clusters; k++) {
		// 2. For each data point x not chosen yet, compute D(x), the distance between x and the nearest center that has already been chosen.
		for (int i = 0; i < particles.size(); i++)
		{
			glm::vec3 current_particle = particles[i].position;
			glm::vec3 closest_center = clusters[particles[i].cluster].centroid;
			float closest_distance = glm::length2(closest_center - current_particle);

			glm::vec3 newest_center = clusters[k].centroid;
			float newest_distance = glm::length2(newest_center - current_particle);
			if (newest_distance < closest_distance) {
				closest_center = newest_center;
				closest_distance = newest_distance;
				particles[i].cluster = k;
			}
			distancesSq.push_back(closest_distance);
		}


		// 3. Choose one new data point at random as a new center, using a weighted probability distribution where a point x is chosen with probability proportional to D(x)^2.
		std::discrete_distribution<int> distance_distribution(distancesSq.begin(), distancesSq.end());
		center_index = distance_distribution(generator);
		clusters[k].centroid = particles[center_index].position;
		distancesSq.clear();
	}
	clock.prettyPrintSinceStart("Algorithm");

	glBindVertexArray(this->renderer_id);
	int size = sizeof(Particle) * particles.size();
	void* data = &particles[0];

	glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
	void* ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	memcpy(ptr, data, size);
	glUnmapBuffer(GL_ARRAY_BUFFER);

	glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
	clock.prettyPrintSinceStart("Upload");
}

/*
* Partly based on ideas from Real Time Moving Average Clustering
* see: https://gregstanleyandassociates.com/whitepapers/BDAC/Clustering/clustering.htm
*
* Complexity - where n is the number of particles, and k is the number of clusters
* Worst case: O(nk),
* Best case: O(n),
**/
void ParticleSystem::rtmac_variant()
{
	Clock clock;
	clock.prettyPrintSinceStart("Start");

	int num_defaults = 0;

	// 1. For each non clustered particle
	for (int i = 0; i < default_cluster.size(); i++)
	{
		if (default_cluster[i] == nullptr) continue;

		num_defaults++;

		Particle* particle = default_cluster[i];
		int closest_index = 1;
		Cluster* closest_center = &clusters[closest_index];
		float closest_distance = glm::length2(closest_center->centroid - particle->position);
		// 2. Find the closest cluster
		for (int c = 2; c < clusters.size(); c++)
		{
			Cluster* current_center = &clusters[c];
			float current_distance = glm::length2(current_center->centroid - particle->position);
			// 3. If the closest cluster is full, look for the next closest cluster
			if (current_distance < closest_distance && current_center->particles.size() < max_cluster_size * 2)
			{
				closest_distance = current_distance;
				closest_center = current_center;
				closest_index = c;
			}
		}

		// 4. If all clusters happened to be full, remove a particle from the cluster
		if (closest_center->particles.size() == max_cluster_size * 2) {
			Particle oldest = closest_center->particles.top();
			particles[oldest.id].cluster = DEFAULT_CLUSTER;
			particles[oldest.id].closest_dist = std::numeric_limits<float>::max();
			default_cluster[oldest.id] = &particles[oldest.id];
			closest_center->particles.pop();
		}

		// 5. Add non clustered particle to closest non full cluster
		particle->cluster = closest_index;
		particle->closest_dist = closest_distance;
		closest_center->particles.push(*particle);
		default_cluster[particle->id] = nullptr;
	}

	// Randomly set some points to default cluster
	for (int i = 0; i < cluster_reset_count; i++)
	{
		int cluster_index = clusters.size() * ((generator() - generator.min()) / (float)(generator.max() - generator.min()));
		if (clusters[cluster_index].particles.size() > 0)
		{
			Particle oldest = clusters[cluster_index].particles.top();
			particles[oldest.id].cluster = DEFAULT_CLUSTER;
			particles[oldest.id].closest_dist = std::numeric_limits<float>::max();
			default_cluster[oldest.id] = &particles[oldest.id];
			clusters[cluster_index].particles.pop();
		}
	}
	std::vector<glm::vec3> center_totals;
	center_totals.resize(clusters.size());
	for (int i = 0; i < particles.size(); i++) {
		center_totals[particles[i].cluster] += particles[i].position;
	}

	for (int c = 0; c < center_totals.size(); c++)
	{
		clusters[c].centroid = center_totals[c] / (float)clusters[c].particles.size();
	}

	std::cout << num_defaults << std::endl;

	clock.prettyPrintSinceStart("Algorithm");

	glBindVertexArray(this->renderer_id);
	int size = sizeof(Particle) * particles.size();
	void* data = &particles[0];

	glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
	void* ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	memcpy(ptr, data, size);
	glUnmapBuffer(GL_ARRAY_BUFFER);

	glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
	clock.prettyPrintSinceStart("Upload");
}