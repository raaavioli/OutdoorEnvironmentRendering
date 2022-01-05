__VERTEX__

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

__FRAGMENT__

#version 410 core
out vec4 color;

layout(location = 0) in vec3 vs_TexCoord;

uniform samplerCube cube_map;

void main() {
  color = texture(cube_map, vs_TexCoord);
}

