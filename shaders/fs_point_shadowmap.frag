#version 450
#include "common.h"

layout(location = 0) in vec3 world_pos_in;
layout(location = 1) flat in uint shadow_map_id_in;

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

void main()
{
    gl_FragDepth = (0.2f + length(world_pos_in - shadow_map_buf.data[shadow_map_id_in].light_pos)) / shadow_map_buf.data[shadow_map_id_in].max_d;
}
