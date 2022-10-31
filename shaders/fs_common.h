#include "common.h"

layout(location = 0) in vec3 norm_in;
layout(location = 1) in vec3 tan_in;
layout(location = 2) in vec3 bitan_in;
layout(location = 3) in vec2 tex_coords;
layout(location = 4) in vec3 world_pos_in;
layout(location = 5) flat in uvec2 tex_ids;
layout(location = 6) in float view_z_in;

layout(location = 0) out vec4 out_col;

layout(constant_id = 0) const uint tex_count = 1;
layout(set = 0, binding = TEX_BINDING) uniform sampler2D textures[tex_count];

layout(constant_id = 1) const uint normal_map_count = 1;
layout(set = 0, binding = NORMAL_MAP_BINDING) uniform sampler2D normal_maps[normal_map_count];

layout(constant_id = 2) const uint dir_shadow_map_count = 1;
layout(set = 0, binding = DIR_SM_BINDING) uniform sampler2DArrayShadow dir_shadow_maps[dir_shadow_map_count];

layout(constant_id = 3) const uint point_shadow_map_count = 1;
layout(set = 0, binding = POINT_SM_BINDING) uniform samplerCube point_shadow_maps[point_shadow_map_count];

struct DirLight
{
    vec3 color;
    vec3 dir;
    uint shadow_map_count;
    uint shadow_map_res_x;
    uint shadow_map_res_y;
    uint shadow_map_id;
};

struct PointLight
{
    vec3 color;
    float max_d;
    vec3 pos;
    uint shadow_map_res;
    float a0;
    float a1;
    float a2;
    uint shadow_map_id;
};

layout(set = 0, binding = DIR_LIGHTS_BINDING) uniform readonly restrict DirLightBuffer
{
    DirLight lights[MAX_DIR_LIGHT_COUNT];
} dir_light_buf;

layout(set = 0, binding = DIR_LIGHTS_VALID_BINDING) uniform usamplerBuffer dir_lights_valid_buf;

layout(set = 0, binding = POINT_LIGHTS_BINDING) uniform readonly restrict PointLightBuffer
{
    PointLight lights[MAX_POINT_LIGHT_COUNT];
} point_light_buf;

layout(set = 0, binding = POINT_LIGHTS_VALID_BINDING) uniform usamplerBuffer point_lights_valid_buf;

struct DirShadowMapData
{
    mat4x4 P;
    mat4x4 tex_P;
    float z;
};

layout(set = 0, binding = DIR_SM_BUF_BINDING) uniform readonly restrict DirShadowMapBuffer
{
    DirShadowMapData data[MAX_DIR_SHADOW_MAP_COUNT * MAX_DIR_SHADOW_MAP_PARTITIONS];
} dir_shadow_map_buf;

void shadeDefault()
{
    const vec3 to_camera = normalize(common_buf.camera_pos - world_pos_in);
    const vec4 object_col = texture(textures[tex_ids[0]], tex_coords);

    if(object_col.a < 0.5f)
    {
        discard;
    }

    const float ka = 0.2f;

    vec3 N;

    if(tex_ids[1] == NORMAL_MAP_ID_NONE)
    {
        N = norm_in;
    }
    else
    {
        N = normalize(2.0f * texture(normal_maps[tex_ids[1]], tex_coords).rgb - 1.0f);

        mat3x3 TBN = mat3x3(tan_in, bitan_in, norm_in);
        N = TBN * N;
    }

    out_col = vec4(0, 0, 0, 0);

    //directional lights
    for(uint i = 0; i < common_buf.dir_light_count; i++)
    {
        const uint valid = texelFetch(dir_lights_valid_buf, int(i)).r;
        if(valid == 0)
        {
            continue;
        }

        const vec4 light_col = vec4(dir_light_buf.lights[i].color, 1.0f);

        out_col += object_col * light_col * ka;

        const vec3 light_dir = dir_light_buf.lights[i].dir;
        const float kd = max(dot(N, -light_dir), 0.0f);

        //TODO: does this even improve anything?
        if(kd == 0.0f)
        {
            continue;
        }

        float shadow_factor;

        if(dir_light_buf.lights[i].shadow_map_count == 0)
        {
            shadow_factor = 1.0f;
        }
        else
        {
            shadow_factor = 0.0f;

            for(uint sm_id = 0; sm_id < dir_light_buf.lights[i].shadow_map_count; sm_id++)
            {
                //TODO: consider setting the shadow_map_id in dir_light_buf to already be a multiple of MAX_DIR_SHADOW_MAP_PARTITIONS to avoid multiplying every time?
                if(view_z_in <= dir_shadow_map_buf.data[dir_light_buf.lights[i].shadow_map_id * MAX_DIR_SHADOW_MAP_PARTITIONS + sm_id].z)
                {
                    vec4 shadow_map_texcoords = vec4(dir_shadow_map_buf.data[dir_light_buf.lights[i].shadow_map_id * MAX_DIR_SHADOW_MAP_PARTITIONS + sm_id].tex_P * vec4(world_pos_in, 1.0f));
                    vec2 t = fract(shadow_map_texcoords.xy * vec2(dir_light_buf.lights[i].shadow_map_res_x, dir_light_buf.lights[i].shadow_map_res_y) - 0.5f);

                    const int kernel_size = 3;

                    for(int offset_y = -(kernel_size / 2); offset_y <= (kernel_size/2); offset_y++)
                    {
                        for(int offset_x = -(kernel_size / 2); offset_x <= (kernel_size/2); offset_x++)
                        {
                            const vec4 shadow_factors = textureGatherOffset(dir_shadow_maps[dir_light_buf.lights[i].shadow_map_id], vec3(shadow_map_texcoords.x, shadow_map_texcoords.y, sm_id), shadow_map_texcoords.z, ivec2(offset_x, offset_y));
                            shadow_factor += mix(mix(shadow_factors[3], shadow_factors[2], t.x), mix(shadow_factors[0], shadow_factors[1], t.x), t.y);
                        }
                    }

                    shadow_factor /= float(kernel_size * kernel_size);

                    break;
                }
            }
        }

        const vec3 r = normalize(reflect(light_dir, N));
        const float ks = pow(max(dot(r, to_camera), 0), 12);

        out_col += shadow_factor * (object_col * light_col * kd + object_col * light_col * ks);
    }

    //point lights
    for(uint i = 0; i < common_buf.point_light_count; i++)
    {
        const uint valid = texelFetch(point_lights_valid_buf, int(i)).r;
        if(valid == 0)
        {
            continue;
        }

        vec3 light_dir = world_pos_in - point_light_buf.lights[i].pos;
        const float d = length(light_dir);

        if(d > point_light_buf.lights[i].max_d)
        {
            continue;
        }

        const float a = 1.0f / (point_light_buf.lights[i].a0 + d * point_light_buf.lights[i].a1 + d * d * point_light_buf.lights[i].a2);
        const vec4 light_col = vec4(point_light_buf.lights[i].color * a, 1.0f);

        out_col += object_col * light_col * ka;

        if(point_light_buf.lights[i].shadow_map_res != 0)
        {
            float shadow_map_d = point_light_buf.lights[i].max_d * texture(point_shadow_maps[point_light_buf.lights[i].shadow_map_id], light_dir).x;

            if(shadow_map_d < d)
            {
                continue;
            }
        }

        light_dir /= d;

        const vec3 r = normalize(reflect(light_dir, N));
        const float ks = pow(max(dot(r, to_camera), 0), 12);
        const float kd = max(dot(N, -light_dir), 0.0f);

        out_col += object_col * light_col * kd + object_col * light_col * ks;
    }
}
