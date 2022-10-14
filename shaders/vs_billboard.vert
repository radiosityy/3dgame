#version 450

layout(location=0) in vec3 center_pos_in;
layout(location=1) in vec2 size_in;
layout(location=2) in vec4 color_in;
layout(location=3) in uvec2 tex_id_in;

layout(location=0) out vec3 center_pos_out;
layout(location=1) out vec2 size_out;
layout(location=2) out vec4 color_out;
layout(location=3) out uvec2 tex_id_out;

void main()
{
    center_pos_out = center_pos_in;
    size_out = size_in;
    color_out = color_in;
    tex_id_out = tex_id_in;
}
