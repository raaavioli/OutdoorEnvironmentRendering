__VERTEX__
#version 430 core

uniform mat4 u_ViewMatrix;
uniform mat4 u_ProjectionMatrix;

uniform ivec3 u_ParticlesPerDim;
uniform vec3 u_SystemBoundsMin;
uniform vec3 u_SystemBoundsMax;
uniform float u_Time; 
uniform float u_WindAmp;
uniform float u_WindFactor;
uniform vec2 u_WindDirection;

layout(binding = 0) uniform sampler2D u_WindTexture;

out VS_OUT {
	vec2 UV;
	vec3 color;
} vs_out;

float rand2(vec2 val){
  return fract(sin(dot(val, vec2(12.9898, 78.233))) * 1234569.0f);
}

float rand(float val){
  return rand2(vec2(val, val));
}

#define PI 3.1415926536f

void main() {
	uint id = gl_InstanceID;
	uint vertex = gl_VertexID;
	uint bottom = (vertex / 2);
	uint left = (vertex % 2);
	float id01 = id / float(u_ParticlesPerDim.x * u_ParticlesPerDim.z);
	float x = (id % u_ParticlesPerDim.x + (rand(id01) - 0.5)) / float(u_ParticlesPerDim.x);
	float z = (id / u_ParticlesPerDim.x + (rand(id01) - 0.5)) / float(u_ParticlesPerDim.z);

	float width = 0.01;
	float height = 0.5;

	vec3 cam_right = vec3(u_ViewMatrix[0].x, u_ViewMatrix[1].x, u_ViewMatrix[2].x);
	vec3 cam_up = vec3(u_ViewMatrix[0].y, u_ViewMatrix[1].y, u_ViewMatrix[2].y);

	vec3 windD1 = vec3(u_WindDirection.x, 0, u_WindDirection.y + 0.5);
	vec3 windD2 = vec3(u_WindDirection.x, 0, u_WindDirection.y - 0.5);
	float windDFreq = (1.0 + sin(PI * u_Time / 10.0f) + rand(id01)) / 2.0f;
	vec3 wind_direction = normalize(windD1 * windDFreq + (1 - windDFreq) * windD2);
	float global_wind_intensity = (sin(u_Time + PI * u_WindFactor * (wind_direction.x * x + wind_direction.z * z) + texture(u_WindTexture, vec2(x, z)).x) + 1.0) / 2.0f;
	float wind_intensity =  0.1 + u_WindAmp * (global_wind_intensity + rand(x + z));
	vec3 wind = bottom * wind_direction * wind_intensity;

	vec3 world_base_pos = u_SystemBoundsMin + (u_SystemBoundsMax - u_SystemBoundsMin) * vec3(x, 0, z);
	vec3 world_top_pos = world_base_pos + cam_up * height + (rand(id01) - 0.5) * 0.2;
	float blade_height = length(world_top_pos - world_base_pos);
	
	world_top_pos = world_base_pos + normalize(world_top_pos - wind - world_base_pos) * blade_height;

	vec3 world_vertex_pos = (1 - bottom) * world_base_pos + bottom * world_top_pos
		+ (left - 0.5f) * cam_right * width;

	vs_out.UV = vec2(left, bottom);
	vs_out.color = vec3(0.2, 0.5, 0.3);

	vec4 view_base_pos = u_ViewMatrix * vec4(world_vertex_pos, 1.0);
	gl_Position = u_ProjectionMatrix * view_base_pos;
} 

__FRAGMENT__

#version 430 core
layout(location = 0) out vec4 out_Color;

layout(binding = 1) uniform sampler2D u_particle_tex;

#define PI 3.14159265f

in VS_OUT {
	vec2 UV;
	vec3 color;
} vs_out;

void main() {
	float ambient = 0.1;
	float shade = pow(vs_out.UV.y, 8.2) * pow(sin(PI * vs_out.UV.x), 0.5);
	vec3 color = vs_out.color;

	out_Color = vec4(ambient + color * shade, 1.0);
	// texture(u_particle_tex, in_UV);
}