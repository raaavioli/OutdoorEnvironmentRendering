#include "particlesystem.h"

#include <iostream>
#include <limits>

#include <glm/gtx/norm.hpp>

#include "clock.h"

ParticleSystem::ParticleSystem(int _num_particles, int _half_cluster_size) : num_particles(_num_particles), half_cluster_size(_half_cluster_size)
{
	generator.seed(0);
	particles.resize(num_particles);
	default_cluster.resize(num_particles);
	num_clusters = (num_particles + half_cluster_size - 1) / half_cluster_size;
	clusters.resize(num_clusters);

	int dim = pow(particles.size(), 1 / 3.f);
	for (int i = 0; i < particles.size(); i++)
	{
		float x = (i % dim) / (float) dim;
		float y	= ((i / dim) % dim) / (float) dim;
		float z = i / (dim * dim) / (float) dim;
		particles[i].id = i;
		particles[i].position = 100.f * glm::vec3(x, y, z);
		particles[i].velocity = { 0, -1.f, 0 };
		particles[i].size = 0.04f * glm::vec2( 1, 1 );
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

void ParticleSystem::draw_cluster_positions(GLuint shader)
{
	std::vector<Particle> cluster_positions(clusters.size());
	for (int i = 0; i < clusters.size(); i++)
	{
		cluster_positions[i].cluster = i;
		cluster_positions[i].position = clusters[i].centroid;
		cluster_positions[i].size = { 0.2, 0.2 };
	}

	GLuint is_cluster_loc = glGetUniformLocation(shader, "is_Cluster");
	glUniform1i(is_cluster_loc, 1);

	glBindVertexArray(this->renderer_id);
	int size = sizeof(Particle) * cluster_positions.size();
	void* data = &cluster_positions[0];

	glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
	void* ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	memcpy(ptr, data, size);
	glUnmapBuffer(GL_ARRAY_BUFFER);

	glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);

	glBindVertexArray(this->renderer_id);
	glDrawArrays(GL_POINTS, 0, cluster_positions.size());
	glBindVertexArray(0);
}

void ParticleSystem::update(float dt)
{
	static float time = 0.0f;
	time += dt;
	int dim = pow(particles.size(), 1 / 3.f);

	for (auto& particle : particles)
	{
		particle.position += dt * particle.velocity;
		if (particle.position.y < 0) {
			particle.position.y += 100.f;
			if (clusters[particle.cluster].particles.size() > 0)
				most_distant_to_default_cluster(&clusters[particle.cluster]);
		}
		particle.closest_dist = glm::length2(particle.position - clusters[particle.cluster].centroid);
	}
}

void ParticleSystem::most_distant_to_default_cluster(Cluster* cluster)
{
	Particle* distant_particle = cluster->particles.top();
	cluster->particles.pop();
	particles[distant_particle->id].cluster = DEFAULT_CLUSTER;
	particles[distant_particle->id].closest_dist = std::numeric_limits<float>::max();
	default_cluster[distant_particle->id] = &particles[distant_particle->id];
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

	// Randomly set some points to default cluster
	for (int i = 0; i < cluster_reset_count; i++)
	{
		int cluster_index = (clusters.size() - 1) * ((generator() - generator.min()) / (float)(generator.max() - generator.min()));
		Cluster* cluster = &clusters[cluster_index];
		if (cluster->particles.size() > 0)
		{
			most_distant_to_default_cluster(cluster);
		}
	}

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
			Cluster* alternative_center = &clusters[c];
			float alternative_distance = glm::length2(alternative_center->centroid - particle->position);
			// 3. If the closest cluster is full, look for the next closest cluster
			if (alternative_distance < closest_distance && alternative_center->particles.size() < half_cluster_size * 2.0f)
			{
				closest_distance = alternative_distance;
				closest_center = alternative_center;
				closest_index = c;
			}
		}

		// 4. If all clusters happened to be full, remove most distant particle in that cluster
		if (closest_center->particles.size() == half_cluster_size * 2.0f) {
			most_distant_to_default_cluster(closest_center);
		}

		// 5. Add non clustered particle to closest non full cluster
		particle->cluster = closest_index;
		particle->closest_dist = closest_distance;
		closest_center->particles.push(particle);
		default_cluster[particle->id] = nullptr;
	}

	std::vector<glm::vec3> cluster_position_sum(clusters.size(), glm::vec3(0));
	for (int i = 0; i < particles.size(); i++) {
		cluster_position_sum[particles[i].cluster] += particles[i].position;
	}

	for (int c = 0; c < cluster_position_sum.size(); c++)
	{
		// Force centers to top if they appear empty and close to bottom (valid for particle systems going downward).
		if (clusters[c].particles.size() < half_cluster_size / 2.0f && clusters[c].centroid.y < 5.0)
			clusters[c].centroid.y = 20.0f;
		// Else set center to mean of cluster particle positions
		else
			clusters[c].centroid = cluster_position_sum[c] / (float)clusters[c].particles.size();
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

void ParticleSystem::naive_k_means_pp()
{
	Clock clock;
	clock.prettyPrintSinceStart("Start");

	num_clusters = (particles.size() + half_cluster_size - 1) / half_cluster_size;

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