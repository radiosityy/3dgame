#version 450
#extension GL_EXT_nonuniform_qualifier : enable
#include "common.h"

layout(quads) in;
layout(equal_spacing) in;
layout(cw) in;

layout(location = 0) patch in uint heightmap_id_in;

layout(location = 0) out vec4 col_out;

layout(set = 0, binding = TERRAIN_HEIGHTMAP_BINDING) uniform sampler2D heightmaps[];

void main()
{
    col_out = common_buf.editor_highlight_color;
    const vec3 world_pos = vec3(gl_in[0].gl_Position[0] + gl_TessCoord[0] * common_buf.terrain_patch_size,
                                texture(heightmaps[heightmap_id_in], gl_TessCoord.xy)[0],
                                gl_in[0].gl_Position[1] + gl_TessCoord[1] * common_buf.terrain_patch_size);

    gl_Position = common_buf.VP * vec4(world_pos, 1.0f);
}
