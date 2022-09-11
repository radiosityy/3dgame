#version 450
#include "common.h"

layout(vertices = 1) out;

layout(location = 0) in float tess_level_in[];
layout(location = 1) in vec4 outer_tess_levels_in[];
layout(location = 2) in uint vertex_offset_in[];

layout(location = 0) patch out float tess_level_out;
layout(location = 1) patch out uint vertex_offset_out;

void main()
{
    tess_level_out = tess_level_in[0];
    vertex_offset_out = vertex_offset_in[0];
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

    gl_TessLevelInner[0] = tess_level_in[0];
    gl_TessLevelInner[1] = tess_level_in[0];

    gl_TessLevelOuter[0] = outer_tess_levels_in[0][0];
    gl_TessLevelOuter[1] = outer_tess_levels_in[0][1];
    gl_TessLevelOuter[2] = outer_tess_levels_in[0][2];
    gl_TessLevelOuter[3] = outer_tess_levels_in[0][3];
}
