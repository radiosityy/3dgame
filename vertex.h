#ifndef VERTEX_H
#define VERTEX_H

#include <vector>
#include "geometry.h"
#include "color.h"
#include <vulkan.h>
#include <ranges>

/*-------------------------------------- vertex constraints --------------------------------------*/

template<class T>
concept Vertex = !std::is_polymorphic_v<T> && (sizeof(T) % 16 == 0);

template<class T>
concept VertexContainer = std::ranges::contiguous_range<T> && requires {typename T::value_type;} && Vertex<typename T::value_type>;

/*----------------------------------------- Vertex Quad ------------------------------------------*/

struct VertexQuad
{
    VertexQuad() = default;
    VertexQuad(const mat3x3& _t, const ColorRGBA& _color, uint32_t _tex_id, uint32_t _layer_id, uint32_t _use_texture)
        : T(_t)
        , color(_color)
        , tex_id(_tex_id)
        , layer_id(_layer_id)
        , use_texture(_use_texture)
    {}

    mat4x4 T;
    ColorRGBA color = {1, 1, 1, 1};
    uint32_t tex_id;
    uint32_t layer_id;
    uint32_t use_texture = 1;
};

inline const std::vector<VkVertexInputAttributeDescription> vertex_quad_attr_desc
{
    {0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0}, // 4x4 transform matrix (mat4x4) - col0
    {1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 16}, // 4x4 transform matrix (mat4x4) - col1
    {2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 32}, // 4x4 transform matrix (mat4x4) - col2
    {3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 48}, // 4x4 transform matrix (mat4x4) - col3
    {4, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 64}, // rgba color
    {5, 0, VK_FORMAT_R32G32B32_UINT, 80}, // tex_id, layer_id, use_texture (uvec3)
};

/*----------------------------------------- Vertex Default ------------------------------------------*/

struct InstanceVertexData
{
    mat4x4 W = glm::identity<mat4x4>();
    uint32_t tex_id = 0;
    uint32_t normal_map_id = 0;
};

struct VertexDefault
{
    vec3 pos;
    vec3 normal;
    vec3 tangent;
    vec2 tex_coords;
    uint32_t bone_id = 0;
};

inline const std::vector<VkVertexInputAttributeDescription> vertex_default_attr_desc
{
    {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0}, // pos
    {1, 0, VK_FORMAT_R32G32B32_SFLOAT, 12}, // normal
    {2, 0, VK_FORMAT_R32G32B32_SFLOAT, 24}, // tangent
    {3, 0, VK_FORMAT_R32G32_SFLOAT, 36}, // texcoords
    {4, 0, VK_FORMAT_R32_UINT, 44}, // bone_id

    {5, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 0}, // world matrix row0
    {6, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 16}, // world matrix row1
    {7, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 32}, // world matrix row2
    {8, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 48}, // world matrix row3
    {9, 1, VK_FORMAT_R32G32_UINT, 64}, // tex_id, normal_map_id
};

/*----------------------------------------- Vertex Terrain ------------------------------------------*/

struct VertexTerrain
{
    vec2 pos;
    float res;
    float lod_res;
    float edge_res_left;
    float edge_res_top;
    float edge_res_right;
    float edge_res_bottom;
    uint32_t vertex_offset;
};

inline const std::vector<VkVertexInputAttributeDescription> vertex_terrain_attr_desc
{
    {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexTerrain, pos)}, // pos
    {1, 0, VK_FORMAT_R32_SFLOAT, offsetof(VertexTerrain, res)}, // res
    {2, 0, VK_FORMAT_R32_SFLOAT, offsetof(VertexTerrain, lod_res)}, // lod res
    {3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VertexTerrain, edge_res_left)}, // edge res
    {4, 0, VK_FORMAT_R32_UINT, offsetof(VertexTerrain, vertex_offset)}, // vertex offset
};

#endif // VERTEX_H