#version 450
#extension GL_EXT_nonuniform_qualifier : enable
#include "common.h"

layout(quads) in;
layout(equal_spacing) in;
layout(cw) in;

layout(location = 0) patch in uint heightmap_id_in;

layout(set = 0, binding = TERRAIN_HEIGHTMAP_BINDING) uniform sampler2D heightmaps[];

void main()
{
    const vec3 world_pos = vec3(gl_in[0].gl_Position[0] + gl_TessCoord[0] * common_buf.terrain_patch_size,
                                texture(heightmaps[heightmap_id_in], gl_TessCoord.xy)[0],
                                gl_in[0].gl_Position[1] + gl_TessCoord[1] * common_buf.terrain_patch_size);

    gl_Position = vec4(world_pos, 1.0f);
}
