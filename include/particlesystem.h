#pragma once

#include <vector>
#include <queue>
#include <random>

#include <glad/glad.h>
#include <glm/glm.hpp>

#define DEFAULT_CLUSTER 0.0f

struct Particle
{
	int id;
	glm::vec3 position;
	glm::vec3 velocity;
	glm::vec2 size;
	float cluster;
	float closest_dist;

	bool operator<(const Particle& o) const
	{
		return this->closest_dist < o.closest_dist;
	}

	bool operator>(const Particle& o) const
	{
		return this->closest_dist > o.closest_dist;
	}
};

struct Cluster
{
	glm::vec3 centroid;
  std::priority_queue<Particle> particles;
};

struct ParticleSystem 
{
public:
	ParticleSystem(int _num_particles, int _max_cluster_size);

	void update(float dt);
	void draw_clusters();
	inline int get_cluster_count() { return num_clusters; }
	inline int get_cluster_count(int cluster) { return clusters[cluster].particles.size(); }

private:
	void naive_k_means_pp();
	void rtmac_variant();

public:
	int cluster_reset_count = 0;

private:
	int num_particles;
	std::vector<Particle> particles;
	std::vector<Particle*> default_cluster;
	std::vector<Cluster> clusters;

	int max_cluster_size;
	int num_clusters;
	GLuint renderer_id, vbo;

	std::default_random_engine generator;
};