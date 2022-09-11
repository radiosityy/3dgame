#version 450
#include "common.h"

layout(triangles) in;
layout(triangle_strip, max_vertices = 3 * MAX_DIR_SHADOW_MAP_PARTITIONS) out;

struct ShadowMapData
{
    mat4x4 P;
    mat4x4 tex_P;
    float z;
};

layout(set = 0, binding = DIR_SM_BUF_BINDING) uniform readonly restrict ShadowMapBuffer
{
    ShadowMapData shadow_maps[455];
} shadow_map_buf;

layout(push_constant) uniform pushConstants
{
    uint shadow_map_count;
    uint shadow_map_offset;
} push_const;

void main()
{
    for(uint i = 0; i < push_const.shadow_map_count; i++)
    {
        gl_Layer = int(i);

        for(uint v = 0; v < 3; v++)
        {
            gl_Position = shadow_map_buf.shadow_maps[push_const.shadow_map_offset + i].P * gl_in[v].gl_Position;
            EmitVertex();
        }

        EndPrimitive();
    }
}
