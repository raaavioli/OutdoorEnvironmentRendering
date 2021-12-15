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

out VS_OUT {
  vec2 size;
} vs_out;

uniform mat4 u_ViewMatrix;

void main() {
  vec4 proj_pos = u_ViewMatrix * vec4(a_Position, 1.0);
  gl_Position = proj_pos; 
  vs_out.size = a_Size;
}
)";

const char* particle_gs_code = R"(
#version 410 core
layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

in VS_OUT {
  vec2 size;
} vs_out[1];

layout (location = 0) out vec2 out_UV;

uniform mat4 u_ProjectionMatrix;

void main() {    
  float half_width = vs_out[0].size.x;
  float half_height = vs_out[0].size.y;
  gl_Position = u_ProjectionMatrix * (gl_in[0].gl_Position + vec4(-half_width, -half_height, 0.0, 0.0));
  out_UV = vec2(0.0, 0.0);    
  EmitVertex();   

  gl_Position = u_ProjectionMatrix * (gl_in[0].gl_Position + vec4( half_width, -half_height, 0.0, 0.0));
  out_UV = vec2(1.0, 0.0);
  EmitVertex();

  gl_Position = u_ProjectionMatrix * (gl_in[0].gl_Position + vec4(-half_width,  half_height, 0.0, 0.0));
  out_UV = vec2(0.0, 1.0);
  EmitVertex();

  gl_Position = u_ProjectionMatrix * (gl_in[0].gl_Position + vec4( half_width,  half_height, 0.0, 0.0));
  out_UV = vec2(1.0, 1.0);
  EmitVertex();

  EndPrimitive();
}  
)";


const char* particle_fs_code = R"(
#version 410 core
out vec4 color;

layout(location = 0) in vec2 in_UV;

void main() {
  color = vec4(in_UV.x, in_UV.y, 0.0, 1.0);
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