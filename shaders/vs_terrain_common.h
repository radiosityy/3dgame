#include "common.h"

layout(location = 0) in vec2 pos_in;
layout(location = 1) in float res_in;
layout(location = 2) in float lod_res_in;
layout(location = 3) in vec4 edge_res_in;
layout(location = 4) in uint vertex_offset_in;

struct VertexData
{
    float height;
    uint tex_id;
};

layout(set = 0, binding = TERRAIN_BINDING) buffer readonly restrict TerrainPerVertexData
{
    VertexData v[];
} vertex_data;

void calcXZVid(uint res_plus1, out uint x, out uint z, out uint vid)
{
    z = gl_VertexIndex / (6 * uint(lod_res_in));
    const uint z_offset = ((gl_VertexIndex + 1) / 3) % 2;
    z += z_offset;

    x = (gl_VertexIndex % (6 * uint(lod_res_in))) / 6;
    const uint x_offset = (((gl_VertexIndex + 2) / 3) + 1) % 2;
    x += x_offset;

    x = uint(float(x) * res_in / lod_res_in);
    z = uint(float(z) * res_in / lod_res_in);

    vid = vertex_offset_in + z * (res_plus1) + x;
}

vec3 posW(uint res_plus1, uint x, uint z, uint vid)
{
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

    const float dx = common_buf.terrain_patch_size_x / res_in;
    const float dz = common_buf.terrain_patch_size_z / res_in;
    return vec3(pos_in.x + float(x) * dx, h, pos_in.y + float(z) * dz);
}

vec3 posW()
{
    const uint res_plus1 = uint(res_in) + 1;
    uint x, z, vid;
    calcXZVid(res_plus1, x, z, vid);
    return posW(res_plus1, x, z, vid);
}
