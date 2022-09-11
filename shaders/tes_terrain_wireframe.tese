#version 450
#include "common.h"

layout(quads) in;

layout(location = 0) patch in float tess_level_in;
layout(location = 1) patch in uint vertex_offset_in;

layout(location = 0) out vec4 col_out;

struct PerVertexData
{
    float height;
    uint tex_id;
    //uint normal_map_id;
};

layout(set = 0, binding = TERRAIN_BINDING) buffer readonly restrict TerrainPerVertexData
{
    PerVertexData v[];
} per_vertex_data;

void main()
{
    const float x = gl_TessCoord.x * tess_level_in;
    const float z = gl_TessCoord.y * tess_level_in;

    const uint tess_level_plus1 = uint(tess_level_in) + 1;

    const float tx = fract(x);
    const float tz = fract(z);

    const uint base_id = vertex_offset_in + uint(z) * tess_level_plus1 + uint(x);

    const float h0 = mix(per_vertex_data.v[base_id].height, per_vertex_data.v[base_id + 1].height, tx);
    const float h1 = mix(per_vertex_data.v[base_id + tess_level_plus1].height, per_vertex_data.v[base_id + tess_level_plus1 + 1].height, tx);
    const float h = mix(h0, h1, tz);

    const vec3 world_pos = vec3(mix(gl_in[0].gl_Position.x, gl_in[0].gl_Position.x + common_buf.terrain_patch_size_x, gl_TessCoord.x), h, mix(gl_in[0].gl_Position.z, gl_in[0].gl_Position.z + common_buf.terrain_patch_size_z, gl_TessCoord.y));
    gl_Position = common_buf.VP * vec4(world_pos, 1.0f);
    col_out = vec4(0.2f, 0.2f, 1.0f, 1.0f);
}
