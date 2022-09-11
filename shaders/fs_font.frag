#version 450
#include "common.h"

layout(constant_id = 0) const int font_count = 1;
layout(set = 0, binding = FONT_BINDING) uniform sampler2DArray samp[font_count];

layout(location = 0) in vec2 tex_coord;
layout(location = 1) flat in uvec3 tex_id;
layout(location = 2) flat in vec4 color;

layout(location = 0) out vec4 out_col;

void main()
{
    out_col = vec4(color.xyz, color.w * texture(samp[tex_id.x], vec3(tex_coord, tex_id.y)).r);
}
