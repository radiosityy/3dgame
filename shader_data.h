#ifndef SHADER_DATA_H
#define SHADER_DATA_H

#include <string>
#include "light.h"
#include <cstring>

/*--------------------------------------- shader constants ---------------------------------------*/
#include "shaders/shader_constants.h"

/*--------------------------------------- shader filenames ---------------------------------------*/

/*--- Vertex Shaders ---*/
constexpr auto VS_QUAD_FILENAME = "shaders/vs_quad.spv";
constexpr auto VS_DEFAULT_FILENAME = "shaders/vs_default.spv";
constexpr auto VS_BILLBOARD_FILENAME = "shaders/vs_billboard.spv";
constexpr auto VS_SHADOWMAP_FILENAME = "shaders/vs_shadowmap.spv";
constexpr auto VS_HIGHLIGHT_FILENAME = "shaders/vs_highlight.spv";
constexpr auto VS_TERRAIN_FILENAME = "shaders/vs_terrain.spv";

/*--- Geometry Shaders ---*/
constexpr auto GS_UI_FILENAME = "shaders/gs_ui.spv";
constexpr auto GS_BILLBOARD_FILENAME = "shaders/gs_billboard.spv";
constexpr auto GS_DIR_SHADOW_MAP_FILENAME = "shaders/gs_dir_shadowmap.spv";
constexpr auto GS_POINT_SHADOW_MAP_FILENAME = "shaders/gs_point_shadowmap.spv";

/*--- Tessellation Shaders ---*/
constexpr auto TCS_TERRAIN_FILENAME = "shaders/tcs_terrain.spv";
constexpr auto TES_TERRAIN_FILENAME = "shaders/tes_terrain.spv";
constexpr auto TES_TERRAIN_SHADOWMAP_FILENAME = "shaders/tes_terrain_shadowmap.spv";
constexpr auto TES_TERRAIN_WIREFRAME_FILENAME = "shaders/tes_terrain_wireframe.spv";

/*--- Fragment Shaders ---*/
constexpr auto FS_UI_FILENAME = "shaders/fs_ui.spv";
constexpr auto FS_FONT_FILENAME = "shaders/fs_font.spv";
constexpr auto FS_DEFAULT_FILENAME = "shaders/fs_default.spv";
constexpr auto FS_BILLBOARD_FILENAME = "shaders/fs_billboard.spv";
constexpr auto FS_POINT_SHADOW_MAP_FILENAME = "shaders/fs_point_shadowmap.spv";
constexpr auto FS_COLOR_FILENAME = "shaders/fs_color.spv";
constexpr auto FS_TERRAIN_EDITOR_FILENAME = "shaders/fs_terrain_editor.spv";

/*--------------------------------------- structures ---------------------------------------*/

struct alignas(16) DirLightShaderData
{
    DirLightShaderData() = default;
    DirLightShaderData(const DirLight& dir_light)
        : color(dir_light.color)
        , dir(dir_light.dir)
        , shadow_map_count(dir_light.shadow_map_count)
        , shadow_map_res_x(dir_light.shadow_map_res_x)
        , shadow_map_res_y(dir_light.shadow_map_res_y)
    {}
    DirLightShaderData& operator=(const DirLight& dir_light)
    {
        color = dir_light.color;
        dir = dir_light.dir;
        shadow_map_count = dir_light.shadow_map_count;
        shadow_map_res_x = dir_light.shadow_map_res_x;
        shadow_map_res_y = dir_light.shadow_map_res_y;
        return *this;
    }

    vec3 color = vec3(0.0f, 0.0f, 0.0f);
    alignas(16) vec3 dir = vec3(0.0f, 0.0f, 0.0f);
    uint32_t shadow_map_count = 0; //0, if light doesn't cast shadows
    uint32_t shadow_map_res_x = 0;
    uint32_t shadow_map_res_y = 0;
    uint32_t shadow_map_id = 0;
};
static_assert(sizeof(DirLight) == sizeof(DirLightShaderData));

struct alignas(16) PointLightShaderData
{
    PointLightShaderData() = default;
    PointLightShaderData(const PointLight& point_light)
        : color(point_light.color * point_light.power)
        , max_d(point_light.max_d)
        , pos(point_light.pos)
        , shadow_map_res(point_light.shadow_map_res)
        , a0(point_light.a0)
        , a1(point_light.a1)
        , a2(point_light.a2)
    {}
    PointLightShaderData& operator=(const PointLight& point_light)
    {
        color = point_light.color * point_light.power;
        max_d = point_light.max_d;
        pos = point_light.pos;
        shadow_map_res = point_light.shadow_map_res;
        a0 = point_light.a0;
        a1 = point_light.a1;
        a2 = point_light.a2;
        return *this;
    }

    vec3 color = vec3(0.0f, 0.0f, 0.0f);
    float max_d = 0.0f;
    vec3 pos = vec3(0.0f, 0.0f, 0.0f);
    uint32_t shadow_map_res = 0; //0, if light doesn't cast shadows
    float a0 = 0.0f;
    float a1 = 0.0f;
    float a2 = 0.0f;
    uint32_t shadow_map_id = 0;
};
static_assert(sizeof(PointLight) == sizeof(PointLightShaderData));

#endif // SHADER_DATA_H
