#version 450

layout(location=0) in mat4x4 T_in;
layout(location=4) in vec4 col_in;
layout(location=5) in uvec3 tex_id_in;

layout(location=0) out mat4x4 T_out;
layout(location=4) out vec4 col_out;
layout(location=5) out uvec3 tex_id_out;

void main()
{
    T_out = T_in;
    col_out = col_in;
    tex_id_out = tex_id_in;
}
