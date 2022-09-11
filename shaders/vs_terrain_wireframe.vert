#version 450
#include "common.h"

layout(location = 0) in vec3 pos_in;
layout(location = 1) in vec3 center_pos_left_in;
layout(location = 2) in vec3 center_pos_bottom_in;
layout(location = 3) in vec3 center_pos_right_in;
layout(location = 4) in vec3 center_pos_top_in;
layout(location = 5) in float tess_level_in;
layout(location = 6) in vec4 outer_tess_levels_in;
layout(location = 7) in uint vertex_offset_in;

layout(location = 0) out float tess_level_out;
layout(location = 1) out vec4 outer_tess_levels_out;
layout(location = 2) out uint vertex_offset_out;

void main()
{
    tess_level_out = tess_level_in;
    outer_tess_levels_out = outer_tess_levels_in;
    vertex_offset_out = vertex_offset_in;
    gl_Position = vec4(pos_in, 1.0f);
}
