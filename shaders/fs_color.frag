#version 450

layout(location = 0) in vec4 col_in;

layout(location = 0) out vec4 out_col;

void main()
{
    out_col = col_in;
}
