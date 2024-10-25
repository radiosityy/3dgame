#version 450
#include "common.h"

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

layout(location=0) in vec2 top_left_pos_in[];
layout(location=1) in vec2 size_in[];
layout(location=2) in vec4 color_in[];
layout(location=3) in uvec3 tex_id_in[];

layout(location = 0) out vec2 tex_coord_out;
layout(location = 1) flat out uvec3 tex_id_out;
layout(location = 2) flat out vec4 color_out;

void main()
{
    const float x0 = top_left_pos_in[0].x;
    const float x1 = top_left_pos_in[0].x + size_in[0].x;
    const float y0 = top_left_pos_in[0].y;
    const float y1 = top_left_pos_in[0].y + size_in[0].y;

    /*--- top left vertex ---*/
    gl_Position = vec4(common_buf.ui_scale * top_left_pos_in[0], 0.0f, 1.0f);
    gl_Position.xy = vec2(2.0f * gl_Position.x - 1.0f, 2.0f * gl_Position.y - 1.0f);
    tex_coord_out = vec2(0.0f, 0.0f);
    tex_id_out = tex_id_in[0];
    color_out = color_in[0];
    EmitVertex();


    /*--- top right vertex ---*/
    gl_Position = vec4(common_buf.ui_scale * vec2(x1, y0), 0.0f, 1.0f);
    gl_Position.xy = vec2(2.0f * gl_Position.x - 1.0f, 2.0f * gl_Position.y - 1.0f);
    tex_coord_out = vec2(1.0f, 0.0f);
    tex_id_out = tex_id_in[0];
    color_out = color_in[0];
    EmitVertex();


    /*--- bottom left vertex ---*/
    gl_Position = vec4(common_buf.ui_scale * vec2(x0, y1), 0.0f, 1.0f);
    gl_Position.xy = vec2(2.0f * gl_Position.x - 1.0f, 2.0f * gl_Position.y - 1.0f);
    tex_coord_out = vec2(0.0f, 1.0f);
    tex_id_out = tex_id_in[0];
    color_out = color_in[0];
    EmitVertex();


    /*--- bottom right vertex ---*/
    gl_Position = vec4(common_buf.ui_scale * vec2(x1, y1), 0.0f, 1.0f);
    gl_Position.xy = vec2(2.0f * gl_Position.x - 1.0f, 2.0f * gl_Position.y - 1.0f);
    tex_coord_out = vec2(1.0f, 1.0f);
    tex_id_out = tex_id_in[0];
    color_out = color_in[0];
    EmitVertex();
}
