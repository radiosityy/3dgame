#ifndef RENDER_DATA_H
#define RENDER_DATA_H

#include <list>
#include <functional>
#include "font.h"
#include <optional>

enum class RenderMode
{
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

enum class RenderModeUi
{
    Ui,
    Font,
    Count
};

constexpr uint32_t FRAMES_IN_FLIGHT = 2;
constexpr uint32_t SMALL_BUFFER_SIZE = 65536;
constexpr uint32_t RENDER_MODE_COUNT = static_cast<uint32_t>(RenderMode::Count);
constexpr uint32_t RENDER_MODE_UI_COUNT = static_cast<uint32_t>(RenderModeUi::Count);

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
};

#endif // RENDER_DATA_H
