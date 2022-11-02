#version 450
#include "common.h"

layout(location = 0) in vec3 pos_in;
layout(location = 4) in uint bone_id_in;
layout(location = 5) in mat4x4 W_in;
layout(location = 10) in uint bone_offset_in;

layout(set = 0, binding = BONE_TRANSFORM_BUF_BINDING) buffer readonly restrict BoneTransformData
{
    mat4x4 Ts[];
} bone_transforms;

void main()
{
    const mat4x4 W = W_in * bone_transforms.Ts[bone_offset_in + bone_id_in];

    gl_Position = W * vec4(pos_in, 1.0f);
}

//unused
layout(location = 1) in vec3 norm_in;
layout(location = 2) in vec3 tan_in;
layout(location = 3) in vec2 tex_coords_in;
layout(location = 9) in uvec2 tex_ids_in;
