#version 450
#include "vs_terrain_common.h"

void main()
{
    gl_Position = vec4(posW(), 1.0f);
}
