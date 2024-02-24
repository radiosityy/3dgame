#version 450
#include "common.h"

layout(vertices = 1) out;

layout(location = 0) in uint heightmap_id_in[];

layout(location = 0) patch out uint heightmap_id_out;

float tessLevel(float d)
{
    //TODO: do this better
    return clamp(MAX_TESS_LEVEL - (d / 10.0f), 1.0f, MAX_TESS_LEVEL);
}

void main()
{
    const vec3 pos = vec3(gl_in[0].gl_Position[0], 0.0f, gl_in[0].gl_Position[1]);
    const float half_terrain_patch_size = 0.5f * common_buf.terrain_patch_size;

    //TODO: consider precomputing midpoints and pass them in vertex shader - check if any performance benefit
    gl_TessLevelOuter[0] = tessLevel(distance(pos + vec3(0.0f, 0.0f, half_terrain_patch_size), common_buf.camera_pos));
    gl_TessLevelOuter[1] = tessLevel(distance(pos + vec3(half_terrain_patch_size, 0.0f, 0.0f), common_buf.camera_pos));
    gl_TessLevelOuter[2] = tessLevel(distance(pos + vec3(common_buf.terrain_patch_size, 0.0f, half_terrain_patch_size), common_buf.camera_pos));
    gl_TessLevelOuter[3] = tessLevel(distance(pos + vec3(half_terrain_patch_size, 0.0f, common_buf.terrain_patch_size), common_buf.camera_pos));

    gl_TessLevelInner[0] = tessLevel(distance(pos + vec3(half_terrain_patch_size, 0.0f, half_terrain_patch_size), common_buf.camera_pos));
    gl_TessLevelInner[1] = gl_TessLevelInner[0];

    heightmap_id_out = heightmap_id_in[0];
    gl_out[gl_InvocationID].gl_Position = gl_in[0].gl_Position;
}
