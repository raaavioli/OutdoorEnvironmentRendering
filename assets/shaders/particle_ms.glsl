__TASK__
#version 450
#extension GL_NV_mesh_shader : require
#extension GL_NV_gpu_shader5 : enable
#extension GL_KHR_shader_subgroup_ballot : enable

struct Particle {
	vec3 position; float width;
	vec3 velocity; float height;
};

layout(std430, binding = 0) buffer ParticleSSBO
{
	Particle particles[];
}; 

layout(location = 1) uniform int u_MeshletCount;

taskNV out Task {
  uint8_t  mesh_invocations;
} OUT;

bool earlyCull(Particle p)
{
    return true;
}

#define GROUP_SIZE 16

layout(local_size_x=GROUP_SIZE) in;
void main() {
    // we padded the buffer to ensure we don't access it out of bounds
    /*Particle particle = particles[gl_GlobalInvocationID.x];

    // implement some early culling function
    bool render = gl_GlobalInvocationID.x < u_MeshletCount && !earlyCull(particle);

    uvec4 vote  = subgroupBallot(render);
    uint  tasks = subgroupBallotBitCount(vote);

    if (gl_LocalInvocationID.x == 0) {
        // write the number of surviving meshlets, i.e. 
        // mesh workgroups to spawn
        gl_TaskCountNV = tasks;

        // where the meshletIDs started from for this task workgroup
        OUT.baseID = tasks;
    }

    {
    // write which children survived into a compact array
    uint idxOffset = subgroupBallotExclusiveBitCount(vote);
    if (render) {
        OUT.subIDs[idxOffset] = uint8_t(gl_LocalInvocationID.x);
    }
    }*/

    bool render = gl_LocalInvocationID.x % 2 == 0 || true;
    uvec4 vote  = subgroupBallot(render);
    uint  tasks = subgroupBallotBitCount(vote);

    if (gl_LocalInvocationID.x == 0) {
        // write the number of surviving meshlets, i.e. 
        // mesh workgroups to spawn
        gl_TaskCountNV = tasks;
        OUT.mesh_invocations = uint8_t(tasks);
    }
}

__MESH__
#version 450
#extension GL_NV_mesh_shader : require
#extension GL_NV_gpu_shader5 : enable
 
taskNV in Task {
    uint8_t  mesh_invocations;
} IN;

out PerVertexData
{
  vec4 color;
} v_out[];   
 
const vec3 vertices[3] = {vec3(-1,-1,0), vec3(1,-1,0), vec3(0,1,0)};
const vec3 colors[3] = {vec3(1.0,0.0,0.0), vec3(0.0,1.0,0.0), vec3(0.0,0.0,1.0)};
 
uniform float u_Scale = 1.0f;
 
layout(local_size_x=3) in;
layout(max_vertices=3, max_primitives=1) out;
layout(triangles) out;
void main()
{
    uint workgroup_id = gl_WorkGroupID.x;
    uint thread_id = gl_LocalInvocationID.x;

    float scale = float(IN.mesh_invocations);

    gl_MeshVerticesNV[thread_id].gl_Position = vec4(vertices[thread_id] / scale + vec3(0, workgroup_id / scale, 0), 1.0);
    v_out[thread_id].color = vec4(colors[thread_id], 1.0);
    gl_PrimitiveIndicesNV[thread_id] = thread_id;
  
    if (gl_LocalInvocationID.x == 0)
	    gl_PrimitiveCountNV = 1;
}

__FRAGMENT__
#version 450
 
layout(location = 0) out vec4 FragColor;
 
in PerVertexData
{
  vec4 color;
} fragIn;  
 
void main()
{
  FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}