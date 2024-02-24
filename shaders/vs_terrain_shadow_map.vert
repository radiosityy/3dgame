#version 450

layout(location = 0) in vec2 pos_in;
layout(location = 1) in uint heightmap_id_in;

layout(location = 0) out uint heightmap_id_out;

void main()
{
    heightmap_id_out = heightmap_id_in;
    gl_Position = vec4(pos_in, 0.0f, 0.0f);
}
