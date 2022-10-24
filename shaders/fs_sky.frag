#version 450
#include "common.h"

layout(early_fragment_tests) in;

layout(location = 0) in vec3 col_in;
layout(location = 1) in vec3 world_pos_in;

layout(location = 0) out vec4 col_out;

vec3 YxyToRgb(vec3 col)
{
    col[0] = 1.0f - exp(-col[0] / 25.0f);
    float ratio = col[0] / col[2];

    //convert to XYZ
    vec3 XYZ;
    XYZ.x = col[1] * ratio;
    XYZ.y = col[0];
    XYZ.z = ratio - XYZ.x - XYZ.y;

    //convert to RGB
    const vec3 r = vec3(3.240479f, -1.53715f, -0.49853f);
    const vec3 g = vec3 (-0.969256f, 1.875991f, 0.041556f);
    const vec3 b = vec3 (0.055684f, -0.204043f, 1.057311f);

    return vec3(dot(r, XYZ), dot(g, XYZ), dot(b , XYZ));
}

void main()
{
    col_out = vec4(YxyToRgb(col_in), 1.0f);
    const float d = distance(vec3(common_buf.visual_sun_pos), world_pos_in);
    const float a = 1.0f / (1.0f + pow(d + 1.0f - common_buf.sun_radius, 128));
//    col_out.xyz += vec3(1.0f, 1.0f, 1.0f) * a;
    col_out.xyz = 0.5f * (col_out.xyz + vec3(2.0f, 2.0f, 2.0f) * a);
}
