#include "shader_constants.h"

layout(set = 0, binding = COMMON_BUF_BINDING) uniform readonly restrict CommonBuffer
{
    mat4x4 VP;
    mat4x4 V;
    vec3 camera_pos;
    vec3 camera_up;
    uint dir_light_count;
    uint point_light_count;
    vec3 visual_sun_pos;
    float sun_radius;
    vec3 effective_sun_pos;
    vec3 cur_pos_terrain;
    uint cur_terrain_intersection;
    vec2 ui_scale;
    float terrain_patch_size;
    vec4 editor_highlight_color;
    float editor_terrain_tool_inner_radius;
    float editor_terrain_tool_outer_radius;
} common_buf;
