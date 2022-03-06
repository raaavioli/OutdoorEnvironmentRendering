__VERTEX__
#version 430 core

uniform mat4 u_ViewMatrix;
uniform mat4 u_ProjectionMatrix;

uniform ivec3 u_ParticlesPerDim;
uniform vec3 u_SystemBoundsMin;
uniform vec3 u_SystemBoundsMax;
uniform float u_Time; 

out VS_OUT {
	vec2 UV;
} vs_out;

float rand(float val){
  return fract(sin(dot(vec2(val, val), vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
	uint id = gl_InstanceID;
	uint vertex = gl_VertexID;
	uint bottom = (vertex / 2);
	uint left = (vertex % 2);
	float id01 = id / float(u_ParticlesPerDim.x * u_ParticlesPerDim.z);
	float x = (id % u_ParticlesPerDim.x) / float(u_ParticlesPerDim.x);
	float z = (id / u_ParticlesPerDim.x) / float(u_ParticlesPerDim.x);

	float width = 0.05;
	float height = 1.0;

	vec3 cam_right = vec3(u_ViewMatrix[0].x, u_ViewMatrix[1].x, u_ViewMatrix[2].x);
	vec3 cam_up = vec3(u_ViewMatrix[0].y, u_ViewMatrix[1].y, u_ViewMatrix[2].y);

	float wind_intensity = bottom * clamp(rand(id01 * z * x), 0.15, 0.2) * (sin(id01 * u_Time) + 1);
	vec3 wind = vec3(wind_intensity, 0.0, wind_intensity);

	vec3 world_base_pos = u_SystemBoundsMin + (u_SystemBoundsMax - u_SystemBoundsMin) * vec3(x + rand(id01 * x * z) - 0.5f, 0, z + rand(id01 * x * z) - 0.5f);
	vec3 world_top_pos = world_base_pos + cam_up * height;
	float blade_height = length(world_top_pos - world_base_pos);
	
	world_top_pos = world_base_pos + normalize(world_top_pos + wind - world_base_pos) * blade_height;

	vec3 world_vertex_pos = (1 - bottom) * world_base_pos + bottom * world_top_pos
		+ (left - 0.5f) * cam_right * width;

	vec4 view_base_pos = u_ViewMatrix * vec4(world_vertex_pos, 1.0);

	vs_out.UV = vec2(left, bottom);

	gl_Position = u_ProjectionMatrix * view_base_pos;
} 

__FRAGMENT__

#version 430 core
layout(location = 0) out vec4 out_Color;

layout(binding = 1) uniform sampler2D u_particle_tex;


in VS_OUT {
	vec2 UV;
} vs_out;

void main() {
	float ambient = 0.1;
	float shade = pow(vs_out.UV.y, 2.2);
	vec3 color = vec3(0.2, 0.5, 0.3);
	out_Color = vec4(ambient + color * shade, 1.0);
	// texture(u_particle_tex, in_UV);
}