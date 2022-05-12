__VERTEX__
#version 430 core

vec2 vertices[] = {
  vec2(-1.0, -1.0),
  vec2(1.0, -1.0),
  vec2(1.0, 1.0),
  vec2(1.0, 1.0),
  vec2(-1.0, 1.0),
  vec2(-1.0, -1.0)
};

layout(location = 0) out vec2 out_UV;

void main() {
  out_UV = (vertices[gl_VertexID] + 1) / 2; 
  gl_Position = vec4(vertices[gl_VertexID], 0.0, 1.0);
}


__FRAGMENT__

#version 430 core
out vec4 out_Color;

layout(location = 0) in vec2 in_UV;

layout(binding = 0) uniform sampler2D u_Texture;

layout(location = 1) uniform int u_DrawDepth;

float zNear = 0.01f;
float zFar = 1000.0f;
float linearlizeDepth(float depth)
{
  float ndc = depth * 2.0f - 1.0;
  return (zNear * zFar / (zFar + depth * (zNear - zFar)));
}

void main() {
  if (u_DrawDepth > 0)
  {
    float depth = linearlizeDepth(texture(u_Texture, in_UV).r) / zFar;
    out_Color = vec4(depth, depth, depth, 1.0);
  }
  else
  {
    out_Color = vec4(texture(u_Texture, in_UV).rgb, 1.0f);
  }
}