__VERTEX__
#version 430 core

uniform mat4 u_ViewMatrix;
uniform mat4 u_ProjectionMatrix;
uniform vec3 u_CameraPosition;

// System settings
uniform ivec3 u_ParticlesPerDim;
uniform vec3 u_SystemBoundsMin;
uniform vec3 u_SystemBoundsMax;
uniform float u_Time;
uniform int u_VerticesPerBlade;

// Wind
uniform float u_WindAmp;
uniform float u_WindFactor;
uniform vec2 u_WindDirection;

layout(binding = 0) uniform sampler2D u_WindTexture;

out VS_OUT {
	vec2 UV;
	vec3 color;
	vec3 normal;
	vec3 world_pos;
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
	uint row = (vertex / 2);
	float row01 = row / float(u_VerticesPerBlade / 2.0f - 1.0f);
	float bottom = pow(row01, 0.25f);
	uint left = (vertex % 2);
	float id01 = id / float(u_ParticlesPerDim.x * u_ParticlesPerDim.z);
	float randId = rand(id01);
	float x = (id % u_ParticlesPerDim.x + (randId - 0.5)) / float(u_ParticlesPerDim.x);
	float z = (id / u_ParticlesPerDim.x + (randId - 0.5)) / float(u_ParticlesPerDim.z);

	float width = (0.001 + randId * 0.015) * (1 - row01);
	float height = 1.0f;

	vec3 windD1 = vec3(u_WindDirection.x, 0, u_WindDirection.y + 0.2);
	vec3 windD2 = vec3(u_WindDirection.x, 0, u_WindDirection.y - 0.2);
	float windDFreq = (1.0 + sin(PI * u_Time / 10.0f)) / 2.0f;
	vec3 wind_direction = normalize(windD1 * windDFreq + (1 - windDFreq) * windD2);
	vec3 bi_wind = cross(wind_direction, vec3(0.0, 1.0, 0.0));
	wind_direction = normalize(wind_direction + bi_wind * 0.35 * (randId - 0.5f));

	float global_wind_cascade0 = (sin(PI * 0.5 * u_Time + 5.0f * u_WindFactor * (x * wind_direction.x + z * wind_direction.z) + pow(texture(u_WindTexture, vec2(x + wind_direction.x, z + wind_direction.z)).x, 1.0f)) + 1.0) / 2.0f;
	float global_wind_cascade1 = (sin(PI * u_Time + 13.0f * u_WindFactor * (x * wind_direction.x + z * wind_direction.z) + pow(texture(u_WindTexture, vec2(x + wind_direction.x, z + wind_direction.z)).x, 1.0f)) + 1.0) / 2.0f;
	float global_wind_intensity = pow((global_wind_cascade0 * global_wind_cascade1), 1.2f);
	float wind_intensity =  0.1 + u_WindAmp * (global_wind_intensity + rand(x + z));
	vec3 wind = bottom * wind_direction * wind_intensity;

	vec3 world_base_pos = u_SystemBoundsMin + (u_SystemBoundsMax - u_SystemBoundsMin) * vec3(x, 0, z);
	vec3 world_top_pos = world_base_pos + vec3(0.0, 1.0, 0.0) * height + (randId - 0.5) * 0.2;
	float blade_height = length(world_top_pos - world_base_pos);

	world_top_pos = world_base_pos + normalize(world_top_pos - wind - world_base_pos) * blade_height;

	vec3 blade_right = normalize(cross((world_top_pos + world_base_pos) / 2.0f - u_CameraPosition, vec3(0, 1, 0)));

	vec3 world_vertex_pos = (1 - bottom) * world_base_pos + bottom * world_top_pos
		+ (left - 0.5f) * blade_right * width;

	vs_out.UV = vec2(left, bottom);
	//vs_out.color = vec3(global_wind_intensity,global_wind_intensity,global_wind_intensity);
	vs_out.color = vec3(245, 222, 179) / 255.0f;

	vec3 normal = cross(blade_right, (world_top_pos - world_base_pos));
	vs_out.normal = (1 - left) * (0.2 * normal - blade_right) + left * (0.2 * normal + blade_right);
	vs_out.world_pos = world_vertex_pos;

	gl_Position = u_ProjectionMatrix * u_ViewMatrix * vec4(world_vertex_pos, 1.0);;
} 

__FRAGMENT__

#version 430 core
layout(location = 0) out vec4 out_Color;

uniform vec3 u_LightDirection;
uniform mat4 u_LightViewProjection;

layout(binding = 1) uniform sampler2D u_particle_tex;
layout(binding = 2) uniform sampler2D u_ShadowMap;

#define PI 3.14159265f

in VS_OUT {
	vec2 UV;
	vec3 color;
	vec3 normal;
	vec3 world_pos;
} vs_out;

void main() {
	float ambient = 0.2;
	vec3 N = normalize(vs_out.normal);
	vec3 L = u_LightDirection;
	float lambert = max(dot(N, L), ambient);

	// Shadow mapping
	vec4 proj_light_pos = u_LightViewProjection * vec4(vs_out.world_pos, 1.0f);
	proj_light_pos = (proj_light_pos / proj_light_pos.w) / 2.0 + 0.5;

	float shadow = 0.0;
	float shadow_bias = max(0.0001 * (1.0 - lambert), 0.00000001); 
	const int sample_radius = 2;
	vec2 pixel_size = 1.0 / textureSize(u_ShadowMap, 0);
	for (int y = -sample_radius; y <= sample_radius; y++)
	{
		for (int x = -sample_radius; x <= sample_radius; x++)
		{
			float map_depth = texture(u_ShadowMap, proj_light_pos.xy + vec2(x, y) * pixel_size).x;
			shadow += map_depth + shadow_bias < proj_light_pos.z ? 0.0 : 1.0; 
		}
	}
	shadow /= (sample_radius + 1) * (sample_radius + 1);
	shadow = clamp(shadow, 0.0, 1.0);
	
	// Color mapping
	float gradient = pow(vs_out.UV.y, 8.2) * pow(sin(PI * vs_out.UV.x), 0.5);
	vec3 color = vs_out.color;
	vec3 dark_color = vec3(236, 193, 111) / 255.0f * ambient;

	vec3 albedo = dark_color * (1 - gradient) + color * gradient;

	out_Color = vec4(max(shadow, ambient) * lambert * albedo, 1.0);
}