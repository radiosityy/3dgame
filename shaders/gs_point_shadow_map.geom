#version 450
#include "common.h"

layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

struct ShadowMapData
{
    mat4x4 P[6];
    vec3 light_pos;
    float max_d;
};

layout(set = 0, binding = POINT_SM_BUF_BINDING) uniform readonly restrict ShadowMapBuffer
{
    ShadowMapData data[163];
} shadow_map_buf;

layout(push_constant) uniform pushConstants
{
    layout(offset = 4) uint shadow_map_id;
} push_const;

layout(location = 0) out vec3 world_pos_out;
//TODO: maybe don't pass it here between shaders but make it accesible to fragment shader via push constants
layout(location = 1) flat out uint shadow_map_id_out;

void main()
{
    for(uint i = 0; i < 6; i++)
    {
        gl_Layer = int(i);

        for(uint v = 0; v < 3; v++)
        {
            shadow_map_id_out = push_const.shadow_map_id;
            world_pos_out = vec3(gl_in[v].gl_Position);
            gl_Position = shadow_map_buf.data[push_const.shadow_map_id].P[i] * gl_in[v].gl_Position;
            EmitVertex();
        }

        EndPrimitive();
    }
}
