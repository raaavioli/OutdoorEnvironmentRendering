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

void main(void)
{
  const float ambient = 0.1;
  const vec3 N = normalize(in_Normal);
  const vec3 L = normalize(u_DirectionalLight);
  const float lambert = max(dot(N, L), 0.0);

  // Shadow mapping
  vec4 proj_light_pos = u_LightViewProjection * in_WorldPosition;
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

  // Blinn phong shading
  vec4 color = in_Color * texture(u_Texture, in_UV);
  out_Color.rgb = (ambient + lambert * shadow) * color.rgb;
  out_Color.a = color.a;
}