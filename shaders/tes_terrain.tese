#version 450
#include "common.h"

layout(quads) in;

layout(location = 0) patch in float tess_level_in;
layout(location = 1) patch in uint vertex_offset_in;

layout(location = 0) out vec3 norm_out;
layout(location = 1) out vec3 tan_out;
layout(location = 2) out vec3 bitan_out;
layout(location = 3) out vec2 tex_coords_out;
layout(location = 4) out vec3 world_pos_out;
layout(location = 5) flat out uvec2 tex_ids_out;
layout(location = 6) out float view_z_out;

struct VertexData
{
    float height;
    uint tex_id;
    //uint normal_map_id;
};

layout(set = 0, binding = TERRAIN_BINDING) buffer readonly restrict TerrainPerVertexData
{
    VertexData v[];
} vertex_data;

void main()
{
    const float x = gl_TessCoord.x * tess_level_in;
    const float z = gl_TessCoord.y * tess_level_in;

    const uint tess_level_plus1 = uint(tess_level_in) + 1;

    const float tx = fract(x);
    const float tz = fract(z);

    const uint base_id = vertex_offset_in + uint(z) * tess_level_plus1 + uint(x);

    const float h0 = mix(vertex_data.v[base_id].height, vertex_data.v[base_id + 1].height, tx);
    const float h1 = mix(vertex_data.v[base_id + tess_level_plus1].height, vertex_data.v[base_id + tess_level_plus1 + 1].height, tx);
    const float h = mix(h0, h1, tz);

    const uint base_id_x_min1 = (uint(x) == 0) ? base_id : (base_id - 1);
    const uint base_id_x_plus1 = (uint(x) == uint(tess_level_in)) ? base_id : (base_id + 1);
    //TODO: do we need to normalize?
    tan_out = normalize(vec3(2.0f * common_buf.terrain_patch_size_x / tess_level_in, (vertex_data.v[base_id_x_plus1].height - vertex_data.v[base_id_x_min1].height), 0.0f));

    const uint base_id_z_min1 = (uint(z) == 0) ? base_id : (base_id - tess_level_plus1);
    const uint base_id_z_plus1 = (uint(z) == uint(tess_level_in)) ? base_id : (base_id + tess_level_plus1);
    //TODO: do we need to normalize?
    bitan_out = normalize(vec3(0.0f, (vertex_data.v[base_id_z_min1].height - vertex_data.v[base_id_z_plus1].height), -2.0f * common_buf.terrain_patch_size_z / tess_level_in));

    //TODO: do we need to normalize?
    norm_out = normalize(cross(tan_out, bitan_out));

    tex_coords_out = vec2(gl_TessCoord.x, 1.0f - gl_TessCoord.y);
    world_pos_out = vec3(mix(gl_in[0].gl_Position.x, gl_in[0].gl_Position.x + common_buf.terrain_patch_size_x, gl_TessCoord.x),
                            h,
                            mix(gl_in[0].gl_Position.z, gl_in[0].gl_Position.z + common_buf.terrain_patch_size_z, gl_TessCoord.y));
    tex_ids_out = uvec2(0, 0);

    view_z_out = (common_buf.V * vec4(world_pos_out, 1.0f)).z;
    gl_Position = common_buf.VP * vec4(world_pos_out, 1.0f);
}
