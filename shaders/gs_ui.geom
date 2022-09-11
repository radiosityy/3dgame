#version 450
#include "common.h"

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

layout(location = 0) in mat4x4 T_in[];
layout(location = 4) in vec4 color_in[];
layout(location = 5) in uvec3 tex_id_in[];

layout(location = 0) out vec2 tex_coord_out;
layout(location = 1) flat out uvec3 tex_id_out;
layout(location = 2) flat out vec4 color_out;

void main()
{
    mat4x4 T = common_buf.guiT * T_in[0];

    /*--- top left vertex ---*/
    gl_Position = T * vec4(0.0f, 0.0f, 1.0f, 1.0f);
    gl_Position.xy = vec2(2.0f * gl_Position.x - 1.0f, 2.0f * gl_Position.y - 1.0f);
    tex_coord_out = vec2(0.0f, 0.0f);
    tex_id_out = tex_id_in[0];
    color_out = color_in[0];
    EmitVertex();


    /*--- top right vertex ---*/
    gl_Position = T * vec4(1.0f, 0.0f, 1.0f, 1.0f);
    gl_Position.xy = vec2(2.0f * gl_Position.x - 1.0f, 2.0f * gl_Position.y - 1.0f);
    tex_coord_out = vec2(1.0f, 0.0f);
    tex_id_out = tex_id_in[0];
    color_out = color_in[0];
    EmitVertex();


    /*--- bottom left vertex ---*/
    gl_Position = T * vec4(0.0f, 1.0f, 1.0f, 1.0f);
    gl_Position.xy = vec2(2.0f * gl_Position.x - 1.0f, 2.0f * gl_Position.y - 1.0f);
    tex_coord_out = vec2(0.0f, 1.0f);
    tex_id_out = tex_id_in[0];
    color_out = color_in[0];
    EmitVertex();


    /*--- bottom right vertex ---*/
    gl_Position = T * vec4(1.0f, 1.0f, 1.0f, 1.0f);
    gl_Position.xy = vec2(2.0f * gl_Position.x - 1.0f, 2.0f * gl_Position.y - 1.0f);
    tex_coord_out = vec2(1.0f, 1.0f);
    tex_id_out = tex_id_in[0];
    color_out = color_in[0];
    EmitVertex();
}
