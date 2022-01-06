__VERTEX__

#version 430 core

layout(location = 0) in vec3 a_Pos;
layout(location = 1) in vec3 a_Velocity;
layout(location = 2) in vec2 a_Size;

out VS_OUT {
  vec2 size;
  vec4 color;
} vs_out;

uniform mat4 u_ViewMatrix;
uniform float u_ClusterCount;
uniform ivec3 u_ParticlesPerDim;
uniform vec3 u_BboxMin;
uniform vec3 u_BboxMax;

// Debug
uniform float u_CurrentCluster;
uniform bool u_ColoredParticles;

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

  return clusters_per_dim.x * (relative_pos.z * clusters_per_dim.y + relative_pos.y) + relative_pos.x;
}

void main() {
  int cluster = get_cluster(a_Pos); 

  vec2 randVec = vec2(cluster, 14.1923);
  vs_out.color.rgb = u_ColoredParticles ? vec3(rand(randVec), rand(1 - randVec), rand(randVec * 3 - 3.1415)) : vec3(1.0, 1.0, 1.0);
  vs_out.size = a_Size;

#ifdef DEBUG
  bool cluster_coloring = u_CurrentCluster == cluster;
  if (u_CurrentCluster == u_ClusterCount)
    vs_out.color.a = 1;
  else if (cluster_coloring)
    vs_out.color.a = 1;
  else
    vs_out.color.a = 0;
#else
  vs_out.color.a = 1;
#endif
  
  gl_Position = u_ViewMatrix * vec4(a_Pos, 1.0);
}

__GEOMETRY__

#version 430 core
layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

in VS_OUT {
  vec2 size;
  vec4 color;
} vs_out[1];

layout(location = 0) out vec4 out_Color;
layout(location = 1) out vec2 out_UV;

layout(location = 0) uniform mat4 u_ProjectionMatrix;
layout(location = 1) uniform int u_DepthCull;

layout(binding = 0) uniform sampler2D u_DepthTexture;

float near = 0.0001f;
float far = 1000.0f;
float linearlizeDepth(float depth)
{
  float z_n = 2.0 * depth - 1.0;
  return 2.0 * near * far / (far + near - z_n * (far - near));
}

void main() {
  vec4 proj_position = u_ProjectionMatrix * gl_in[0].gl_Position;
  proj_position /= proj_position.w;
  float particle_depth = linearlizeDepth(proj_position.z);
  float scene_depth = linearlizeDepth(texture(u_DepthTexture, (proj_position.xy + vec2(1f)) / vec2(2f)).x);
  if (u_DepthCull == 0 || particle_depth < scene_depth) 
  {
    float half_width = vs_out[0].size.x;
    float half_height = vs_out[0].size.y;
    gl_Position = u_ProjectionMatrix * (gl_in[0].gl_Position + vec4(-half_width, -half_height, 0.0, 0.0));
    out_UV = vec2(0.0, 1.0);
    out_Color = vs_out[0].color;    
    EmitVertex();   

    gl_Position = u_ProjectionMatrix * (gl_in[0].gl_Position + vec4( half_width, -half_height, 0.0, 0.0));
    out_UV = vec2(1.0, 1.0);
    out_Color = vs_out[0].color;
    EmitVertex();

    gl_Position = u_ProjectionMatrix * (gl_in[0].gl_Position + vec4(-half_width,  half_height, 0.0, 0.0));
    out_UV = vec2(0.0, 0.0);
    out_Color = vs_out[0].color;
    EmitVertex();

    gl_Position = u_ProjectionMatrix * (gl_in[0].gl_Position + vec4( half_width,  half_height, 0.0, 0.0));
    out_UV = vec2(1.0, 0.0);
    out_Color = vs_out[0].color;
    EmitVertex();

    EndPrimitive();
  }
}  

__FRAGMENT__

#version 430 core
layout(location = 0) out vec4 out_Color;

layout(location = 0) in vec4 in_Color;
layout(location = 1) in vec2 in_UV;

layout(binding = 1) uniform sampler2D u_particle_tex;

void main() {
  out_Color = in_Color * texture(u_particle_tex, in_UV);
}