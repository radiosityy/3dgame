#ifndef SHADER_DATA_H
#define SHADER_DATA_H

#include <string>

/*--------------------------------------- shader constants ---------------------------------------*/
#include "shaders/shader_constants.h"

/*--------------------------------------- shader filenames ---------------------------------------*/

/*--- Vertex Shaders ---*/
constexpr auto VS_QUAD_FILENAME = "shaders/vs_quad.spv";
constexpr auto VS_DEFAULT_FILENAME = "shaders/vs_default.spv";
constexpr auto VS_SKY_FILENAME = "shaders/vs_sky.spv";
constexpr auto VS_SHADOWMAP_FILENAME = "shaders/vs_shadow_map.spv";
constexpr auto VS_HIGHLIGHT_FILENAME = "shaders/vs_highlight.spv";
constexpr auto VS_TERRAIN_FILENAME = "shaders/vs_terrain.spv";
constexpr auto VS_TERRAIN_WIREFRAME_FILENAME = "shaders/vs_terrain_wireframe.spv";

/*--- Geometry Shaders ---*/
constexpr auto GS_UI_FILENAME = "shaders/gs_ui.spv";
constexpr auto GS_BILLBOARD_FILENAME = "shaders/gs_billboard.spv";
constexpr auto GS_DIR_SHADOW_MAP_FILENAME = "shaders/gs_dir_shadow_map.spv";
constexpr auto GS_POINT_SHADOW_MAP_FILENAME = "shaders/gs_point_shadow_map.spv";

/*--- Tesselation Shaders ---*/
constexpr auto TCS_TERRAIN_FILENAME = "shaders/tcs_terrain.spv";
constexpr auto TCS_TERRAIN_WIREFRAME_FILENAME = "shaders/tcs_terrain_wireframe.spv";
constexpr auto TES_TERRAIN_FILENAME = "shaders/tes_terrain.spv";
constexpr auto TES_TERRAIN_WIREFRAME_FILENAME = "shaders/tes_terrain_wireframe.spv";

/*--- Fragment Shaders ---*/
constexpr auto FS_UI_FILENAME = "shaders/fs_ui.spv";
constexpr auto FS_FONT_FILENAME = "shaders/fs_font.spv";
constexpr auto FS_DEFAULT_FILENAME = "shaders/fs_default.spv";
constexpr auto FS_SKY_FILENAME = "shaders/fs_sky.spv";
//TODO: do we need dir shadow map fragment shader? it doesn't do anything and maybe we can still get depth writes without it?
//in case it's not needed we can remove this shader file and remove this shader from the pipeline
constexpr auto FS_DIR_SHADOW_MAP_FILENAME = "shaders/fs_dir_shadow_map.spv";
constexpr auto FS_POINT_SHADOW_MAP_FILENAME = "shaders/fs_point_shadow_map.spv";
constexpr auto FS_COLOR_FILENAME = "shaders/fs_color.spv";
constexpr auto FS_TERRAIN_EDITOR_FILENAME = "shaders/fs_terrain_editor.spv";

#endif // SHADER_DATA_H
