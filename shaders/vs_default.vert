#version 450
#include "common.h"

layout(location = 0) in vec3 pos_in;
layout(location = 1) in vec3 norm_in;
layout(location = 2) in vec3 tan_in;
layout(location = 3) in vec2 tex_coords_in;
layout(location = 4) in uint bone_id_in;
layout(location = 5) in mat4x4 W_in;
layout(location = 9) in uvec2 tex_ids_in;

layout(location = 0) out vec3 norm_out;
layout(location = 1) out vec3 tan_out;
layout(location = 2) out vec3 bitan_out;
layout(location = 3) out vec2 tex_coords_out;
layout(location = 4) out vec3 world_pos_out;
layout(location = 5) out uvec2 tex_ids_out;
layout(location = 6) out float view_z_out;

layout(set = 0, binding = BONE_TRANSFORM_BUF_BINDING) buffer readonly restrict BoneTransformData
{
    mat4x4 Ts[];
} bone_transforms;

void main()
{
    const mat4x4 W = W_in * bone_transforms.Ts[bone_id_in];

    norm_out = normalize(vec3(vec4(norm_in, 0.0f) * inverse(W)));
    tan_out = normalize(vec3(vec4(tan_in, 0.0f) * inverse(W)));
    bitan_out = normalize(cross(norm_out, tan_out));
    tex_coords_out = tex_coords_in;
    tex_ids_out = tex_ids_in;

    world_pos_out = vec3(W * vec4(pos_in, 1.0f));
    view_z_out = (common_buf.V * vec4(world_pos_out, 1.0f)).z;
    gl_Position = common_buf.VP * vec4(world_pos_out, 1.0f);
}
