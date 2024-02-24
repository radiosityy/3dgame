#version 450
#include "vs_terrain_common.h"

layout(location = 0) out vec4 col_out;

void main()
{
    gl_Position = common_buf.VP * vec4(posW(), 1.0f);
    col_out = vec4(0.2f, 0.2f, 1.0f, 1.0f);
}
