__VERTEX__
#version 430 core

layout(location = 0) in vec3 a_Pos;
layout(location = 1) in vec4 a_VertexColor;
layout(location = 2) in vec3 a_Normal;
layout(location = 3) in vec2 a_UV;

layout(location = 0) out vec2 o_UV;
layout(location = 1) out vec4 o_Color;
layout(location = 2) out vec3 o_Normal;
layout(location = 3) out vec4 o_WorldPosition;

layout(location = 0) uniform mat4 u_Model;
layout(location = 1) uniform mat4 u_ViewProjection;
layout(location = 2) uniform vec4 u_ModelColor;

void main(void)
{
  o_UV = a_UV;
  o_Color = a_VertexColor * u_ModelColor;
  o_Normal = (u_Model * vec4(a_Normal, 0.0)).xyz;
  o_WorldPosition = u_Model * vec4(a_Pos, 1.0);
  gl_Position = u_ViewProjection * o_WorldPosition;
}

__FRAGMENT__
#version 430 core
layout(location = 0) in vec2 in_UV;
layout(location = 1) in vec4 in_Color;
layout(location = 2) in vec3 in_Normal;
layout(location = 3) in vec4 in_WorldPosition;

layout(location = 0) out vec4 out_Color;

layout(binding = 0) uniform sampler2D u_Texture;
layout(binding = 1) uniform sampler2D u_ShadowMap;

layout(location = 3) uniform vec3 u_DirectionalLight = vec3(-1.0, 1.0, -1.0);
layout(location = 4) uniform mat4 u_LightViewProjection;
layout(location = 5) uniform float u_ShadowBias = 0.00001;

void main(void)
{
  float ambient = 0.2;

  // Shadow mapping
  vec4 light_pos = u_LightViewProjection * in_WorldPosition;
  light_pos /= light_pos.w;
  light_pos = light_pos / 2.0f + 0.5f;
  vec4 map_depth = texture(u_ShadowMap, light_pos.xy);
  float shadow = map_depth.x < light_pos.z - u_ShadowBias ? ambient : 1; 

  // Blinn phong shading
  float lambert = clamp(dot(u_DirectionalLight, in_Normal), ambient, shadow);

  vec4 texture = texture(u_Texture, in_UV);
  out_Color.rgb = lambert * in_Color.rgb * texture.rgb;
  out_Color.a = in_Color.a * texture.a;
}