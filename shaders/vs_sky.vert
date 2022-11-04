#version 450
#include "common.h"

layout(location = 0) in vec3 pos_in;
layout(location = 1) in vec3 norm_in;
layout(location = 2) in vec3 tan_in;
layout(location = 3) in vec2 tex_coords_in;
layout(location = 4) in uint bone_id_in;
layout(location = 5) in mat4x4 W_in;
layout(location = 9) in uvec2 tex_ids_in;
layout(location = 10) in uint bone_offset_in;

layout(location = 0) out vec3 col_Yxy_out;
layout(location = 1) out vec3 world_pos_out;

layout(set = 0, binding = BONE_TRANSFORM_BUF_BINDING) buffer readonly restrict BoneTransformData
{
    mat4x4 Ts[];
} bone_transforms;

vec3 perezZenith(float t, float theta_sun)
{
    const float pi = 3.1415926f;
    const vec4 cx1 = vec4(0.0f, 0.00209f, -0.00375f, 0.00165f);
    const vec4 cx2 = vec4(0.00394f, -0.03202f, 0.06377f, -0.02903f);
    const vec4 cx3 = vec4(0.25886f, 0.06052f, -0.21196f, 0.11693f);
    const vec4 cy1 = vec4(0.0f, 0.00317f, -0.00610f, 0.00275f);
    const vec4 cy2 = vec4(0.00516f, -0.04153f, 0.08970f, -0.04214f);
    const vec4 cy3 = vec4(0.26688f, 0.06670f, -0.26756f, 0.15346f);

    const float t2 = t * t ;
    const float chi = (4.0f/9.0f - t/120.0f) * (pi - 2.0f*theta_sun);
    const vec4 theta = vec4(1.0f, theta_sun, theta_sun*theta_sun, theta_sun*theta_sun*theta_sun);

    float Y = (4.0453f*t - 4.9710f) * tan(chi) - 0.2155f*t + 2.4192f;
    float x = t2*dot(cx1 ,theta) + t*dot(cx2 ,theta) + dot(cx3 ,theta);
    float y = t2*dot(cy1 ,theta) + t*dot(cy2 ,theta) + dot(cy3 ,theta);

    return vec3(Y, x, y);
}

vec3 perezFunc(float t, float cos_theta, float cos_gamma)
{
    const float gamma = acos(cos_gamma);
    const float cosGammaSq = cos_gamma * cos_gamma;

    const float aY = 0.17872f * t - 1.46303f;
    const float bY = -0.35540f * t + 0.42749f;
    const float cY = -0.02266f * t + 5.32505f;
    const float dY = 0.12064f * t - 2.57705f;
    const float eY = -0.06696f * t + 0.37027f;

    const float ax = -0.01925f * t - 0.25922f;
    const float bx = -0.06651f * t + 0.00081f;
    const float cx = -0.00041f * t + 0.21247f;
    const float dx = -0.06409f * t - 0.89887f;
    const float ex = -0.00325f * t + 0.04517f;

    const float ay = -0.01669f * t - 0.26078f;
    const float by = -0.09495f * t + 0.00921f;
    const float cy = -0.00792f * t + 0.21023f;
    const float dy = -0.04405f * t - 1.65369f;
    const float ey = -0.01092f * t + 0.05291f;

    return vec3 ((1.0f + aY * exp(bY/cos_theta)) * (1.0f + cY * exp(dY*gamma) + eY * cosGammaSq),
                 (1.0f + ax * exp(bx/cos_theta)) * (1.0f + cx * exp(dx*gamma) + ex * cosGammaSq),
                 (1.0f + ay * exp(by/cos_theta)) * (1.0f + cy * exp(dy*gamma) + ey * cosGammaSq));
}

vec3 perezSky(float t, float cos_theta, float cos_gamma, float cos_theta_sun)
{
    const float theta_sun = acos(cos_theta_sun);
    const vec3 zenith = perezZenith(t, theta_sun);
    vec3 col_Yxy = zenith * perezFunc(t, cos_theta, cos_gamma) / perezFunc(t, 1.0f, cos_theta_sun);

    col_Yxy[0] *= smoothstep(0.0f, 0.1f, cos_theta_sun);

    return col_Yxy;
}

void main()
{
    const mat4x4 W = W_in * bone_transforms.Ts[bone_id_in];

    const vec4 world_pos = W * vec4(pos_in, 1.0f);

    vec3 v = normalize(world_pos.xyz);
    v.y += 0.0001f;

    const vec3 l = common_buf.effective_sun_pos;

    world_pos_out = v;

    col_Yxy_out = perezSky(1.7f, v.y, clamp(dot(v, l), 0.0f, 1.0f), l.y);

    gl_Position = common_buf.VP * world_pos;
}
