#version 450
#include "common.h"

layout(vertices = 1) out;

layout(location = 0) in vec3 center_pos_left_in[];
layout(location = 1) in vec3 center_pos_bottom_in[];
layout(location = 2) in vec3 center_pos_right_in[];
layout(location = 3) in vec3 center_pos_top_in[];
layout(location = 4) in float tess_level_in[];
layout(location = 5) in vec4 outer_tess_levels_in[];
layout(location = 6) in uint vertex_offset_in[];

layout(location = 0) patch out float tess_level_out;
layout(location = 1) patch out uint vertex_offset_out;

float tessLevel(vec3 center_pos, float max_tess_level)
{
    const float d = distance(common_buf.camera_pos, center_pos);

    float tess_level = clamp(2.0f, max_tess_level, max_tess_level * (50.0f / d));
    tess_level = float(1 << uint(log2(tess_level) + 0.5f));

    return tess_level;
}

void main()
{
    tess_level_out = tess_level_in[0];
    vertex_offset_out = vertex_offset_in[0];
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

    //TODO: store the center pos in the vertex as well?
    const vec3 center_pos = (center_pos_left_in[0] + center_pos_bottom_in[0] + center_pos_right_in[0] + center_pos_top_in[0]) / 4.0f;
    const float inner_tess_level = tessLevel(center_pos, tess_level_in[0]);

    gl_TessLevelInner[0] = inner_tess_level;
    gl_TessLevelInner[1] = inner_tess_level;

    gl_TessLevelOuter[0] = tessLevel(center_pos_left_in[0], outer_tess_levels_in[0][0]);
    gl_TessLevelOuter[1] = tessLevel(center_pos_bottom_in[0], outer_tess_levels_in[0][1]);
    gl_TessLevelOuter[2] = tessLevel(center_pos_right_in[0], outer_tess_levels_in[0][2]);
    gl_TessLevelOuter[3] = tessLevel(center_pos_top_in[0], outer_tess_levels_in[0][3]);
}
