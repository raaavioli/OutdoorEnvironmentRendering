#pragma once

#include <vector>
#include <queue>
#include <random>

#include <glad/glad.h>
#include <glm/glm.hpp>

#define DEFAULT_CLUSTER 0.0f

struct Particle
{
	glm::vec3 position;
	glm::vec3 velocity;
	glm::vec2 size;
};

struct ParticleSystem 
{
public:
	ParticleSystem::ParticleSystem(glm::ivec3 particles_per_dim, glm::vec3 bbox_min, glm::vec3 bbox_max);

	void update(float dt);
	void draw();
	inline int get_num_clusters() { return num_clusters; }
	inline glm::vec3 get_bbox_min() { return bbox_min; }
	inline glm::vec3 get_bbox_max() { return bbox_min; }
	inline glm::ivec3 get_particles_per_dim() { return particles_per_dim; }

private:
	int get_cluster(glm::vec3 position);

private:
	// System data
	std::vector<Particle> particles;

	// System dimensions
	glm::vec3 bbox_min;
	glm::vec3 bbox_max;
	glm::ivec3 particles_per_dim;
	int num_clusters;

	// 5 * 5 * 5 = 125 ~ 128 particles per cluster
	const int particles_per_cluster_dim = 5;
	const int particles_per_cluster = 125;

	GLuint renderer_id, vbo;
};