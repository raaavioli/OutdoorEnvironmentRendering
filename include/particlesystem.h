#pragma once

#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

struct Particle
{
	glm::vec3 position;
	glm::vec3 velocity;
	glm::vec2 size;
	float cluster;
};

struct ParticleSystem 
{
public:
	ParticleSystem(int num_particles);

	void draw_clusters();
	inline int get_cluster_count() { return num_clusters; }

private:
	void clusterize();

private:
	int num_particles;
	std::vector<Particle> particles;
	int num_clusters;
	int approx_cluster_size;
	GLuint renderer_id, vbo;
};