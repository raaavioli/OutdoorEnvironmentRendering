#ifndef WR_SHADER_H
#define WR_SHADER_H

/**
 * 
 * Temporary file for storing shaders not to bloat main
 * 
 */

const char* texture_vs_code = R"(
#version 410 core
layout(location = 0) in vec2 a_Pos;
layout(location = 1) in vec2 a_TextureData;

layout(location = 0) out vec2 vs_UV;

void main() {
  gl_Position = vec4(a_Pos, 0.0, 1.0); 
}
)";

const char* texture_fs_code = R"(
#version 410 core
out vec4 color;

layout(location = 0) in vec2 vs_UV;

uniform sampler2D texture0;

void main() {
  color = texture(texture0, vs_UV);
}
)";

const char* particle_vs_code = R"(
#version 410 core
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_Size;

#define DEBUG 1

out VS_OUT {
  vec2 size;
  vec4 color;
  bool cluster_debug;
} vs_out;

uniform mat4 u_ViewMatrix;
uniform float u_ClusterCount;
uniform ivec3 u_ParticlesPerDim;
uniform vec3 u_BboxMin;
uniform vec3 u_BboxMax;

// Debug
uniform float u_CurrentCluster;
uniform bool is_Cluster;

float rand(vec2 co){
  return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

int get_cluster(vec3 position)
{
  int particles_per_cluster_dim = 5;
	ivec3 relative_pos = ivec3((u_ParticlesPerDim) * (position - u_BboxMin) / (u_BboxMax - u_BboxMin));
	relative_pos = min(relative_pos, u_ParticlesPerDim - 1);
	relative_pos = relative_pos / particles_per_cluster_dim;

	ivec3 clusters_per_dim = (u_ParticlesPerDim + particles_per_cluster_dim - 1) / particles_per_cluster_dim;

	int cluster = clusters_per_dim.x * (relative_pos.z * clusters_per_dim.y + relative_pos.y) + relative_pos.x;
	return cluster;
}

void main() {
  vec4 proj_pos = u_ViewMatrix * vec4(a_Position, 1.0);

  int cluster = get_cluster(a_Position.xyz);
  float cluster01 = cluster / u_ClusterCount;
  vec2 randVec = vec2(cluster, 14.1923);

  vs_out.size = a_Size;
  vs_out.color.rgb = is_Cluster ? vec3(1.0, 0, 0) : vec3(rand(randVec), rand(1 - randVec), rand(randVec * 3 - 3.1415));

#ifdef DEBUG
  bool cluster_coloring = u_CurrentCluster == cluster;
  vs_out.cluster_debug = cluster_coloring;
  if (u_CurrentCluster == u_ClusterCount)
    vs_out.color.a = 1;
  else if (cluster_coloring)
    vs_out.color.a = 1;
  else
    vs_out.color.a = 0;
#else
  vs_out.color.a = 1;
  vs_out.cluster_debug = false;
#endif
  
  gl_Position = proj_pos;
}
)";

const char* particle_gs_code = R"(
#version 410 core
layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

in VS_OUT {
  vec2 size;
  vec4 color;
  bool cluster_debug;
} vs_out[1];

layout (location = 0) out vec4 out_Color;

uniform mat4 u_ProjectionMatrix;

void main() {    
  float half_width = vs_out[0].size.x;
  float half_height = vs_out[0].size.y;
  gl_Position = u_ProjectionMatrix * (gl_in[0].gl_Position + vec4(-half_width, -half_height, 0.0, 0.0));
  if (vs_out[0].cluster_debug) {
    gl_Position /= gl_Position.w;
    gl_Position.z = 0;
  }
  out_Color = vs_out[0].color;    
  EmitVertex();   

  gl_Position = u_ProjectionMatrix * (gl_in[0].gl_Position + vec4( half_width, -half_height, 0.0, 0.0));
  if (vs_out[0].cluster_debug) {
    gl_Position /= gl_Position.w;
    gl_Position.z = 0;
  }
  out_Color = vs_out[0].color;
  EmitVertex();

  gl_Position = u_ProjectionMatrix * (gl_in[0].gl_Position + vec4(-half_width,  half_height, 0.0, 0.0));
  if (vs_out[0].cluster_debug) {
    gl_Position /= gl_Position.w;
    gl_Position.z = 0;
  }
  out_Color = vs_out[0].color;
  EmitVertex();

  gl_Position = u_ProjectionMatrix * (gl_in[0].gl_Position + vec4( half_width,  half_height, 0.0, 0.0));
  if (vs_out[0].cluster_debug) {
    gl_Position /= gl_Position.w;
    gl_Position.z = 0;
  }
  out_Color = vs_out[0].color;
  EmitVertex();

  EndPrimitive();
}  
)";


const char* particle_fs_code = R"(
#version 410 core
out vec4 color;

layout(location = 0) in vec4 in_Color;

void main() {
  color = in_Color;
}
)";


const char* skybox_vs_code = R"(
#version 410 core
layout(location = 0) in vec3 a_Pos;

layout(location = 0) out vec3 vs_TexCoord;

uniform mat4 u_ViewProjection;

void main() {
  vs_TexCoord = a_Pos;
  vec4 proj_pos = u_ViewProjection * vec4(a_Pos, 1.0);

  // To render skybox where z = 1 (at far clipping plane). Rasterization does "gl_Position.xyz / gl_Position.w"
  gl_Position = proj_pos.xyww;  
}
)";

const char* skybox_fs_code = R"(
#version 410 core
out vec4 color;

layout(location = 0) in vec3 vs_TexCoord;

uniform samplerCube cube_map;

void main() {
  color = texture(cube_map, vs_TexCoord);
}
)";


#endif // WR_SHADERS_H