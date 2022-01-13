__VERTEX__
#version 430 core

layout(location = 0) in vec3 a_Pos;
layout(location = 1) in vec4 a_VertexColor;
layout(location = 2) in vec3 a_Normal;
layout(location = 3) in vec2 a_UV;

layout(location = 0) out vec4 o_Color;

layout(location = 0) uniform mat4 u_Model;
layout(location = 1) uniform mat4 u_ViewProjection;
layout(location = 2) uniform vec4 u_ModelColor;

void main(void)
{
  o_Color = a_VertexColor * u_ModelColor;
  gl_Position = u_ViewProjection * u_Model * vec4(a_Pos, 1.0);
}

__FRAGMENT__
#version 430 core
layout(location = 0) in vec4 in_Color;

layout(location = 0) out vec4 out_Color;

layout(binding = 0) uniform sampler2D u_Texture;

void main(void)
{
  out_Color = in_Color;
}