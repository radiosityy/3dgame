#version 450

layout(location = 0) in vec3 pos_in;
layout(location = 1) in vec3 center_pos_left_in;
layout(location = 2) in vec3 center_pos_bottom_in;
layout(location = 3) in vec3 center_pos_right_in;
layout(location = 4) in vec3 center_pos_top_in;
layout(location = 5) in float tess_level_in;
layout(location = 6) in vec4 outer_tess_levels_in;
layout(location = 7) in uint vertex_offset_in;

layout(location = 0) out vec3 center_pos_left_out;
layout(location = 1) out vec3 center_pos_bottom_out;
layout(location = 2) out vec3 center_pos_right_out;
layout(location = 3) out vec3 center_pos_top_out;
layout(location = 4) out float tess_level_out;
layout(location = 5) out vec4 outer_tess_levels_out;
layout(location = 6) out uint vertex_offset_out;

void main()
{
    //norm_out = normalize(vec3(vec4(norm_in, 0.0f) * inverse(W_in)));
    //tan_out = normalize(vec3(vec4(tan_in, 0.0f) * inverse(W_in)));
    //tex_coords_out = tex_coords_in;
    //tex_ids_out = tex_ids_in;

    //world_pos_out = vec3(W_in * vec4(pos_in, 1.0f));
    //view_z_out = (push_const.V * vec4(world_pos_out, 1.0f)).z;
    //gl_Position = push_const.VP * vec4(world_pos_out, 1.0f);
    center_pos_left_out = center_pos_left_in;
    center_pos_bottom_out = center_pos_bottom_in;
    center_pos_right_out = center_pos_right_in;
    center_pos_top_out = center_pos_top_in;
    tess_level_out = tess_level_in;
    outer_tess_levels_out = outer_tess_levels_in;
    vertex_offset_out = vertex_offset_in;
    gl_Position = vec4(pos_in, 1.0f);
}
