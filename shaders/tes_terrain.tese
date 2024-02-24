#version 450
#extension GL_EXT_nonuniform_qualifier : enable
#include "common.h"

layout(quads) in;
layout(equal_spacing) in;
layout(cw) in;

layout(location = 0) patch in uint heightmap_id_in;

layout(location = 0) out vec3 norm_out;
layout(location = 1) out vec3 tan_out;
layout(location = 2) out vec3 bitan_out;
layout(location = 3) out vec2 tex_coords_out;
layout(location = 4) out vec3 world_pos_out;
layout(location = 5) flat out uvec2 tex_ids_out;
layout(location = 6) out float view_z_out;

struct VertexData
{
    uint tex_id;
};

layout(set = 0, binding = TERRAIN_BUF_BINDING) buffer readonly restrict TerrainPerVertexData
{
    VertexData v[];
} vertex_data;

layout(set = 0, binding = TERRAIN_HEIGHTMAP_BINDING) uniform sampler2D heightmaps[];

void main()
{
    world_pos_out = vec3(gl_in[0].gl_Position[0] + gl_TessCoord[0] * common_buf.terrain_patch_size,
                         texture(heightmaps[heightmap_id_in], gl_TessCoord.xy)[0],
                         gl_in[0].gl_Position[1] + gl_TessCoord[1] * common_buf.terrain_patch_size);

    tex_coords_out = vec2(gl_TessCoord[0], 1.0f - gl_TessCoord[1]);
    tex_ids_out = uvec2(0, 0);

    const float tex_coord_d = 1.0f / MAX_TESS_LEVEL;
    const float patch_d = common_buf.terrain_patch_size / MAX_TESS_LEVEL;

    tan_out = normalize(vec3(2.0f * patch_d,
                             texture(heightmaps[heightmap_id_in], gl_TessCoord.xy + vec2(tex_coord_d, 0.0f))[0] - texture(heightmaps[heightmap_id_in], gl_TessCoord.xy - vec2(tex_coord_d, 0.0f))[0],
                             0.0f));

    bitan_out = normalize(vec3(0.0f,
                               texture(heightmaps[heightmap_id_in], gl_TessCoord.xy + vec2(0.0f, tex_coord_d))[0] - texture(heightmaps[heightmap_id_in], gl_TessCoord.xy - vec2(0.0f, tex_coord_d))[0],
                               2.0f * patch_d));

    norm_out = cross(bitan_out, tan_out);

    view_z_out = (common_buf.V * vec4(world_pos_out, 1.0f)).z;
    gl_Position = common_buf.VP * vec4(world_pos_out, 1.0f);
}
