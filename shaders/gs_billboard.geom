#version 450
#include "common.h"

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

layout(location=0) in vec3 center_pos_in[];
layout(location=1) in vec2 size_in[];
layout(location=2) in vec4 color_in[];
layout(location=3) in uvec2 tex_id_in[];

layout(location = 0) out vec2 tex_coord_out;
layout(location = 1) flat out uvec2 tex_id_out;
layout(location = 2) flat out vec4 color_out;

void main()
{
//    const vec3 up = vec3(0.0f, 1.0f, 0.0f) * size_in[0].y;
    const vec3 up = common_buf.camera_up * size_in[0].y;
    const vec3 right = normalize(cross(up, common_buf.camera_pos - center_pos_in[0])) * size_in[0].x;

    /*--- top left vertex ---*/
    gl_Position = common_buf.VP * vec4(-0.5f * right + 0.5f * up + center_pos_in[0], 1.0f);
    tex_coord_out = vec2(0.0f, 0.0f);
    tex_id_out = tex_id_in[0];
    color_out = color_in[0];
    EmitVertex();


    /*--- top right vertex ---*/
    gl_Position = common_buf.VP * vec4(0.5f * right + 0.5f * up + center_pos_in[0], 1.0f);
    tex_coord_out = vec2(1.0f, 0.0f);
    tex_id_out = tex_id_in[0];
    color_out = color_in[0];
    EmitVertex();


    /*--- bottom left vertex ---*/
    gl_Position = common_buf.VP * vec4(-0.5f * right + -0.5f * up + center_pos_in[0], 1.0f);
    tex_coord_out = vec2(0.0f, 1.0f);
    tex_id_out = tex_id_in[0];
    color_out = color_in[0];
    EmitVertex();


    /*--- bottom right vertex ---*/
    gl_Position = common_buf.VP * vec4(0.5f * right + -0.5f * up + center_pos_in[0], 1.0f);
    tex_coord_out = vec2(1.0f, 1.0f);
    tex_id_out = tex_id_in[0];
    color_out = color_in[0];
    EmitVertex();
}
