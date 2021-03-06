__VERTEX__
#version 430 core

layout(location = 0) in vec3 a_Pos;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_UV;

layout(location = 0) out vec3 o_Color;
layout(location = 1) out vec2 o_UV;
layout(location = 2) out vec3 o_WorldNormal;
layout(location = 3) out vec4 o_WorldPosition;
layout(location = 4) flat out uint o_EntityID;

// Environment uniforms
layout(location = 0) uniform mat4 u_ViewProjection;

// Instance uniforms
layout(location = 1) uniform uint u_EntityID;
layout(location = 2) uniform mat4 u_Model;

// Material uniforms (identical to FRAGMENT)
layout(location = 3) uniform vec3 u_Color;
layout(binding = 1) uniform sampler2D u_AlbedoMap;


void main(void)
{
	o_Color = u_Color;
	o_UV = a_UV;
	o_WorldNormal = (u_Model * vec4(a_Normal, 0.0)).xyz;
	o_WorldPosition = u_Model * vec4(a_Pos, 1.0);
	o_EntityID = u_EntityID;

	gl_Position = u_ViewProjection * o_WorldPosition;
}

__FRAGMENT__
#version 430 core
layout(location = 0) in vec3 in_Color;
layout(location = 1) in vec2 in_UV;
layout(location = 2) in vec3 in_WorldNormal;
layout(location = 3) in vec4 in_WorldPosition;
layout(location = 4) flat in uint in_EntityID;

layout(location = 0) out vec4 out_Color;
layout(location = 1) out uint out_Id;

// Material uniforms (identical to VERTEX)
layout(location = 3) uniform vec3 u_Color;
layout(binding = 1) uniform sampler2D u_AlbedoMap;

// Lighting uniforms
layout(binding = 0) uniform sampler2D u_ShadowMap;
layout(location = 6) uniform vec3 u_DirectionalLight = vec3(-1.0, 1.0, -1.0);
layout(location = 7) uniform mat4 u_LightViewProjection;
layout(location = 8) uniform float u_MinVariance = 0.00001f;

float linstep(float min, float max, float v)
{
	return clamp((v - min) / (max - min), 0, 1);
} 

float ReduceLightBleeding(float p_max, float amount) 
{   
	// Remove the [0, amount] tail and linearly rescale (amount, 1].    
	return linstep(amount, 1, p_max); 
}

float ChebyshevUpperBound(vec2 moments, float t) 
{   
	// One-tailed inequality valid if t > Moments.x    
	float p = float(t <= moments.x);
	float variance = moments.y - (moments.x * moments.x);
	variance = max(variance, u_MinVariance); 
	
	// Probabilistic upper bound.  
	float d = t - moments.x;
	float p_max = variance / (variance + d * d);
	float bleed_threshold = max(1 - dot(u_DirectionalLight, vec3(0.0, 1.0, 0.0)), 0.0);
	return ReduceLightBleeding(max(p_max, p), 0.05);
} 

/**
* Gaussian blur
* Based upon: https://github.com/Jam3/glsl-fast-gaussian-blur
*/
vec2 blur5(sampler2D image, vec2 uv, vec2 direction) {
	vec2 resolution = vec2(textureSize(image, 0));
	vec2 color = vec2(0.0);
	vec2 off1 = vec2(1.3333333333333333) * direction;
	color += texture2D(image, uv).xy * 0.29411764705882354;
	color += texture2D(image, uv + (off1 / resolution)).xy * 0.35294117647058826;
	color += texture2D(image, uv - (off1 / resolution)).xy * 0.35294117647058826;
	return color; 
}


vec2 blur13(sampler2D image, vec2 uv, vec2 direction) {
	vec2 resolution = vec2(textureSize(image, 0));
	vec2 color = vec2(0.0);
	vec2 off1 = vec2(1.411764705882353) * direction;
	vec2 off2 = vec2(3.2941176470588234) * direction;
	vec2 off3 = vec2(5.176470588235294) * direction;
	color += texture2D(image, uv).xy * 0.1964825501511404;
	color += texture2D(image, uv + (off1 / resolution)).xy * 0.2969069646728344;
	color += texture2D(image, uv - (off1 / resolution)).xy * 0.2969069646728344;
	color += texture2D(image, uv + (off2 / resolution)).xy * 0.09447039785044732;
	color += texture2D(image, uv - (off2 / resolution)).xy * 0.09447039785044732;
	color += texture2D(image, uv + (off3 / resolution)).xy * 0.010381362401148057;
	color += texture2D(image, uv - (off3 / resolution)).xy * 0.010381362401148057;
	return color;
}

float ShadowContribution(vec2 uv, float distance_to_light) 
{   
#if 1
	vec2 moments = blur13(u_ShadowMap, uv, vec2(1.0, 0.0));
#else
	vec2 moments = texture2D(u_ShadowMap, uv).xy;
#endif

	// Compute the Chebyshev upper bound.    
	return ChebyshevUpperBound(moments, distance_to_light); 
} 

void main(void)
{
	const float ambient = 0.1;
	const vec3 N = normalize(in_WorldNormal);
	const vec3 L = normalize(u_DirectionalLight);
	const float lambert = max(dot(N, L), 0.0);

	// Variance Shadow Mapping
	vec4 proj_light_pos = u_LightViewProjection * in_WorldPosition;
	proj_light_pos = (proj_light_pos / proj_light_pos.w) / 2.0 + 0.5;

	float shadow = ShadowContribution(proj_light_pos.xy, proj_light_pos.z);
	shadow = clamp(shadow, 0.0, 1.0);

	vec3 color = in_Color * texture(u_AlbedoMap, in_UV).xyz;
	out_Color = vec4((ambient + lambert * shadow) * color, 1.0);
	out_Id = in_EntityID;
}