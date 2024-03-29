#version 450

layout(location=0) in vec2 top_left_pos_in;
layout(location=1) in vec2 size_in;
layout(location=2) in vec4 color_in;
layout(location=3) in uvec3 tex_id_in;

layout(location=0) out vec2 top_left_pos_out;
layout(location=1) out vec2 size_out;
layout(location=2) out vec4 color_out;
layout(location=3) out uvec3 tex_id_out;

void main()
{
    top_left_pos_out = top_left_pos_in;
    size_out = size_in;
    color_out = color_in;
    tex_id_out = tex_id_in;
}
