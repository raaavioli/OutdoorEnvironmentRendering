__COMPUTE__
#version 430 core
#define PI 3.14159265

/* Timing */
uniform float u_time_delta;
uniform float u_time;

/* Dimensions */
uniform int u_num_particles;
uniform vec3 u_BboxMin;
uniform vec3 u_BboxMax;
uniform ivec3 u_ParticlesPerDim;

struct Particle {
	vec3 position; float width;
	vec3 velocity; float height;
};

layout(std430, binding = 1) buffer ParticleSSBO
{
	Particle particles[];
};

float rand(vec2 co){
	return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

layout(local_size_x = 1024, local_size_y = 1, local_size_z = 1) in;
void main(void) {
	uint id = gl_GlobalInvocationID.x;
	if (id >= u_num_particles) return;

	uint x = id % u_ParticlesPerDim.x;
	uint y = (id / u_ParticlesPerDim.x) % u_ParticlesPerDim.y;
	uint z = (id / (u_ParticlesPerDim.x * u_ParticlesPerDim.y)) % u_ParticlesPerDim.z;

	vec2 rA = vec2(float(id) / float(u_num_particles) + u_time, 2.1923);
	vec2 rB = vec2(float(id) / float(u_num_particles) + u_time, 14.1923);
	vec2 rC = vec2(float(id) / float(u_num_particles) + u_time, 8.1923);

	float max_speed = 0.5;
	Particle particle = particles[id];
	particle.velocity += u_time_delta * vec3(rand(rA) - 0.5, - rand(rB) * 9.82 * u_time_delta, rand(rC) - 0.5);
	particle.velocity = clamp(particle.velocity, vec3(-max_speed, -5 * max_speed, -max_speed), vec3(max_speed, 0, max_speed));
	particle.position += u_time_delta * particle.velocity;

	if ((particle.position.y < u_BboxMin.y) ||
		(particle.position.x < u_BboxMin.x || particle.position.x > u_BboxMax.x) ||
		(particle.position.z < u_BboxMin.z || particle.position.z > u_BboxMax.z)) {
		particle.position = u_BboxMin + (u_BboxMax - u_BboxMin) * vec3(x, u_ParticlesPerDim.y, z) / vec3(u_ParticlesPerDim);
		particle.position.y = u_BboxMax.y;
		particle.velocity = vec3(0, -0.5, 0);
	}

	particles[id] = particle;
}