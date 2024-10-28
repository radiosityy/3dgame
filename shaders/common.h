#include "shader_constants.h"

layout(set = 0, binding = COMMON_BUF_BINDING) uniform readonly restrict CommonBuffer
{
    mat4x4 VP;
    mat4x4 V;
    vec3 camera_pos;
    float editor_terrain_tool_inner_radius;
    vec3 camera_up;
    float editor_terrain_tool_outer_radius;
    vec3 cur_pos_terrain;
    vec2 ui_scale;
    float terrain_patch_size;
    vec4 editor_highlight_color;
    uint dir_light_count;
    uint point_light_count;
    uint cur_terrain_intersection;
} common_buf;
