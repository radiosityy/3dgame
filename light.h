#ifndef LIGHT_H
#define LIGHT_H

#include "geometry.h"
#include <fstream>

struct alignas(16) DirLight
{
    vec3 color;
    alignas(16) vec3 dir;
    uint32_t shadow_map_count; //0, if light doesn't cast shadows
    uint32_t shadow_map_res_x;
    uint32_t shadow_map_res_y;
};

struct PointLight
{
    PointLight() = default;

    PointLight(std::ifstream& scene_file)
    {
        scene_file.read(reinterpret_cast<char*>(this), sizeof(PointLight));
    }

    void serialize(std::ofstream& outfile) const
    {
        outfile.write(reinterpret_cast<const char*>(this), sizeof(PointLight));
    }

    vec3 color;
    float max_d;
    vec3 pos;
    uint32_t shadow_map_res = 0; //0, if light doesn't cast shadows
    float a0;
    float a1;
    float a2;
    float power;
};

#if 0
struct alignas(16) SpotLight
{
    vec3 color;
    float specular;
    vec2 pos;
    float a0;
    float a1;
    float max_distance;
};
#endif

#endif // LIGHT_H
