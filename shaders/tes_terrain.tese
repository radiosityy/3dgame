#version 450
#include "vs_terrain_common.h"

layout(location = 0) out vec3 norm_out;
layout(location = 1) out vec3 tan_out;
layout(location = 2) out vec3 bitan_out;
layout(location = 3) out vec2 tex_coords_out;
layout(location = 4) out vec3 world_pos_out;
layout(location = 5) flat out uvec2 tex_ids_out;
layout(location = 6) out float view_z_out;

void main()
{
    const uint res_plus1 = uint(res_in) + 1;
    uint x,z,vid;
    calcXZVid(res_plus1, x, z, vid);

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

    world_pos_out = posW(res_plus1, x, z, vid);
    view_z_out = (common_buf.V * vec4(world_pos_out, 1.0f)).z;
    gl_Position = common_buf.VP * vec4(world_pos_out, 1.0f);
}
