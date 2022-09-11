#version 450
#include "common.h"

layout(constant_id = 0) const int tex_count = 1;
layout(set = 0, binding = TEX_BINDING) uniform sampler2D samp[tex_count];

layout(location = 0) in vec2 tex_coord;
layout(location = 1) flat in uvec3 tex_id;
layout(location = 2) flat in vec4 color;

layout(location = 0) out vec4 out_col;

void main()
{
    if(tex_id.z != 0)
    {
        out_col = color * texture(samp[tex_id.x], tex_coord);
    }
    else
    {
        out_col = color;
    }
}
