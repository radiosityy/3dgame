#ifndef RENDER_DATA_H
#define RENDER_DATA_H

#include <list>
#include <functional>
#include "light.h"
#include "font.h"
#include <optional>
#include <cstring>

enum class RenderMode
{
    Ui,
    Font,
    Default,
    Sky,
    Terrain,
    DirShadowMap,
    PointShadowMap,
    TerrainDirShadowMap,
    TerrainPointShadowMap,
    Highlight,
    Billboard,
#if EDITOR_ENABLE
    TerrainWireframe,
#endif
    Count
};

struct alignas(16) DirLightShaderData
{
    DirLightShaderData() = default;
    DirLightShaderData(const DirLight& dir_light)
    {
        //TODO(HACK): we should probably copy field by field, but this works (I think?)
        std::memcpy(this, &dir_light, sizeof(DirLight) - 8);
    }
    DirLightShaderData& operator=(const DirLight& dir_light)
    {
        //TODO(HACK): we should probably copy field by field, but this works (I think?)
        std::memcpy(this, &dir_light, sizeof(DirLight) - 8);
        return *this;
    }

    vec3 color;
    alignas(16) vec3 dir;
    uint32_t shadow_map_count; //0, if light doesn't cast shadows
    uint32_t shadow_map_res_x;
    uint32_t shadow_map_res_y;
    uint32_t shadow_map_id;
};
static_assert(sizeof(DirLight) == sizeof(DirLightShaderData));

struct alignas(16) PointLightShaderData
{
    PointLightShaderData() = default;
    PointLightShaderData(const PointLight& point_light)
    {
        //TODO(HACK): we should probably copy field by field, but this works (I think?)
        std::memcpy(this, &point_light, sizeof(PointLight) - 4);
        color = color * point_light.power;
    }
    PointLightShaderData& operator=(const PointLight& point_light)
    {
        //TODO(HACK): we should probably copy field by field, but this works (I think?)
        std::memcpy(this, &point_light, sizeof(PointLight) - 4);
        color = color * point_light.power;
        return *this;
    }

    vec3 color;
    float max_d;
    vec3 pos;
    uint32_t shadow_map_res = 0; //0, if light doesn't cast shadows
    float a0;
    float a1;
    float a2;
    uint32_t shadow_map_id;
};
static_assert(sizeof(PointLight) == sizeof(PointLightShaderData));

constexpr uint32_t FRAMES_IN_FLIGHT = 2;
constexpr uint32_t SMALL_BUFFER_SIZE = 65536;
constexpr uint32_t RENDER_MODE_COUNT = static_cast<uint32_t>(RenderMode::Count);

struct RenderData
{
    vec3 visual_sun_pos;
    vec3 effective_sun_pos;
    float sun_radius;
    //TODO:remove this from here and make all classes setup render data, push constants etc. by themselves (as part of render modes code?)
    vec3 cur_terrain_pos;
    bool cur_terrain_intersection;
    vec4 editor_highlight_color;
    float editor_terrain_tool_inner_radius;
    float editor_terrain_tool_outer_radius;
    float terrain_patch_size_x;
    float terrain_patch_size_z;
};

struct SceneInitData
{
    std::vector<const Font*> fonts;
    RenderData render_data;
};

#endif // RENDER_DATA_H
