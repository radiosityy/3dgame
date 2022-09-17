#version 450
#include "common.h"

layout(location = 0) in vec2 pos_in;
layout(location = 1) in float res_in;
layout(location = 2) in float lod_res_in;
layout(location = 3) in vec4 edge_res_in;
layout(location = 4) in uint vertex_offset_in;

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
};

layout(set = 0, binding = TERRAIN_BINDING) buffer readonly restrict TerrainPerVertexData
{
    VertexData v[];
} vertex_data;

void main()
{
    uint z = gl_VertexIndex / (6 * uint(lod_res_in));
    const uint z_offset = ((gl_VertexIndex + 1) / 3) % 2;
    z += z_offset;

    uint x = (gl_VertexIndex % (6 * uint(lod_res_in))) / 6;
    const uint x_offset = (((gl_VertexIndex + 2) / 3) + 1) % 2;
    x += x_offset;

    x = uint(float(x) * res_in / lod_res_in);
    z = uint(float(z) * res_in / lod_res_in);

    const uint res_plus1 = uint(res_in) + 1;
    const uint vid = vertex_offset_in + z * (res_plus1) + x;

    //------------------------
    const uint base_id_x_min1 = (uint(x) == 0) ? vid : (vid - 1);
    const uint base_id_x_plus1 = (uint(x) == uint(res_in)) ? vid : (vid + 1);
    //TODO: do we need to normalize?
    tan_out = normalize(vec3(2.0f * common_buf.terrain_patch_size_x / res_in, (vertex_data.v[base_id_x_plus1].height - vertex_data.v[base_id_x_min1].height), 0.0f));

    const uint base_id_z_min1 = (uint(z) == 0) ? vid : (vid - res_plus1);
    const uint base_id_z_plus1 = (uint(z) == uint(res_in)) ? vid : (vid + res_plus1);
    //TODO: do we need to normalize?
    bitan_out = normalize(vec3(0.0f, (vertex_data.v[base_id_z_min1].height - vertex_data.v[base_id_z_plus1].height), -2.0f * common_buf.terrain_patch_size_z / res_in));

    //TODO: do we need to normalize?
    norm_out = normalize(cross(tan_out, bitan_out));

    tex_coords_out = vec2(float(x) / res_in, 1.0f - float(z) / res_in);
    tex_ids_out = uvec2(0, 0);

    //height---------------
    float h = vertex_data.v[vid].height;

    if(z == 0)
    {
        const uint vertex_stride = uint(res_in) / uint(edge_res_in[3]);

        if((x % vertex_stride) != 0)
        {
            const uint left_x = x - (x % vertex_stride);
            const uint right_x = left_x + vertex_stride;
            const float t = float(x - left_x) / float(vertex_stride);
            h = mix(vertex_data.v[vid - (x - left_x)].height, vertex_data.v[vid + (right_x - x)].height, t);
        }
    }
    else if(z == uint(res_in))
    {
        const uint vertex_stride = uint(res_in) / uint(edge_res_in[1]);

        if((x % vertex_stride) != 0)
        {
            const uint left_x = x - (x % vertex_stride);
            const uint right_x = left_x + vertex_stride;
            const float t = float(x - left_x) / float(vertex_stride);
            h = mix(vertex_data.v[vid - (x - left_x)].height, vertex_data.v[vid + (right_x - x)].height, t);
        }
    }

    if(x == 0)
    {
        const uint vertex_stride = uint(res_in) / uint(edge_res_in[0]);

        if((z % vertex_stride) != 0)
        {
            const uint bottom_z = z - (z % vertex_stride);
            const uint top_z = bottom_z + vertex_stride;
            const float t = float(z - bottom_z) / float(vertex_stride);
            h = mix(vertex_data.v[vid - (z - bottom_z) * res_plus1].height, vertex_data.v[vid + (top_z - z) * res_plus1].height, t);
        }
    }
    else if(x == uint(res_in))
    {
        const uint vertex_stride = uint(res_in) / uint(edge_res_in[2]);

        if((z % vertex_stride) != 0)
        {
            const uint bottom_z = z - (z % vertex_stride);
            const uint top_z = bottom_z + vertex_stride;
            const float t = float(z - bottom_z) / float(vertex_stride);
            h = mix(vertex_data.v[vid - (z - bottom_z) * res_plus1].height, vertex_data.v[vid + (top_z - z) * res_plus1].height, t);
        }
    }

    //height---------------

    const float dx = common_buf.terrain_patch_size_x / res_in;
    const float dz = common_buf.terrain_patch_size_z / res_in;
    world_pos_out = vec3(pos_in.x + float(x) * dx, h, pos_in.y + float(z) * dz);
    view_z_out = (common_buf.V * vec4(world_pos_out, 1.0f)).z;
    gl_Position = common_buf.VP * vec4(world_pos_out, 1.0f);
}
