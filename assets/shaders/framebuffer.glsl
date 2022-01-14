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

float zNear = 0.0001f;
float zFar = 1000.0f;
float linearlizeDepth(float depth)
{
  return zNear * zFar / (zFar + depth * (zNear - zFar));
}

void main() {
  if (u_DrawDepth > 0)
    out_Color = vec4(vec3(linearlizeDepth(texture(u_Texture, in_UV).r)), 1.0);
  else
    out_Color = texture(u_Texture, in_UV);
}