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
  out_UV = (vertices[gl_VertexID] + 1) / 2.0f; 
  gl_Position = vec4(vertices[gl_VertexID], 0.0f, 1.0f);
}

__FRAGMENT__
#version 430 core
layout(location = 0) in vec2 in_UV;

layout(location = 0) out vec2 out_Moments;

layout(binding = 0) uniform sampler2D u_DepthTexture;

vec2 ComputeMoments(float depth)
{   
	vec2 moments;   
	// First moment is the depth itself.   
	moments.x = depth;   
	// Compute partial derivatives of depth.    
	float dx = dFdx(depth);   
	float dy = dFdy(depth);   
	// Compute second moment over the pixel extents.   
	moments.y = depth*depth + 0.25*(dx*dx + dy*dy);   
	return moments; 
} 

void main(void)
{
	out_Moments = ComputeMoments(texture(u_DepthTexture, in_UV).x);
}