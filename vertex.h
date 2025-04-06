#ifndef VERTEX_H
#define VERTEX_H

#include <vector>
#include "geometry.h"
#include "color.h"
#include <vulkan/vulkan.h>

//TODO: see if there's any benefit to limiting the number of locations for each vertex format
//there are cases where we can for instance pack all uints into a single location even if they have nothing to do with each other
//and I currently keep them at separate locations for clarity

/*----------------------------------------- Vertex Ui ------------------------------------------*/

struct VertexUi
{
    VertexUi() = default;
    VertexUi(const vec2& top_left_pos_, const vec2& size_, const ColorRGBA& _color, uint32_t _tex_id, uint32_t _layer_id, uint32_t _use_texture)
        : top_left_pos(top_left_pos_)
        , size(size_)
        , color(_color)
        , tex_id(_tex_id)
        , layer_id(_layer_id)
        , use_texture(_use_texture)
    {}

    vec2 top_left_pos;
    vec2 size;
    ColorRGBA color = {1, 1, 1, 1};
    uint32_t tex_id;
    uint32_t layer_id;
    uint32_t use_texture = 1;
};

inline const std::vector<VkVertexInputAttributeDescription> vertex_ui_attr_desc
{
    {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexUi, top_left_pos)}, // top left pos
    {1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexUi, size)}, // size
    {2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VertexUi, color)}, // rgba color
    {3, 0, VK_FORMAT_R32G32B32_UINT, offsetof(VertexUi, tex_id)}, // tex_id, layer_id, use_texture (uvec3)
};

/*----------------------------------------- Vertex Billboard ------------------------------------------*/

struct VertexBillboard
{
    VertexBillboard() = default;
    VertexBillboard(const vec3& center_pos_, const vec2& size_, uint32_t tex_id_, uint32_t layer_id_, const ColorRGBA& color_)
        : center_pos(center_pos_)
        , size(size_)
        , color(color_)
        , tex_id(tex_id_)
        , layer_id(layer_id_)
    {}

    vec3 center_pos;
    vec2 size;
    ColorRGBA color = {1, 1, 1, 1};
    uint32_t tex_id;
    uint32_t layer_id;
};

inline const std::vector<VkVertexInputAttributeDescription> vertex_billboard_attr_desc
{
    {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexBillboard, center_pos)}, // center pos
    {1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexBillboard, size)}, // size
    {2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VertexBillboard, color)}, // color
    {3, 0, VK_FORMAT_R32G32_UINT, offsetof(VertexBillboard, tex_id)}, // tex_id
};

/*----------------------------------------- Vertex Default ------------------------------------------*/

struct InstanceVertexData
{
    mat4x4 W = glm::identity<mat4x4>();
    uint32_t tex_id = 0;
    uint32_t normal_map_id = 0;
    uint32_t bone_offset = 0;
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
    {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexDefault, pos)}, // pos
    {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexDefault, normal)}, // normal
    {2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexDefault, tangent)}, // tangent
    {3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexDefault, tex_coords)}, // texcoords
    {4, 0, VK_FORMAT_R32_UINT, offsetof(VertexDefault, bone_id)}, // bone_id

    {5, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(InstanceVertexData, W)}, // world matrix row0
    {6, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(InstanceVertexData, W) + 4*sizeof(float)}, // world matrix row1
    {7, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(InstanceVertexData, W) + 8*sizeof(float)}, // world matrix row2
    {8, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(InstanceVertexData, W) + 12*sizeof(float)}, // world matrix row3
    {9, 1, VK_FORMAT_R32G32_UINT, offsetof(InstanceVertexData, tex_id)}, // tex_id, normal_map_id
    {10, 1, VK_FORMAT_R32_UINT, offsetof(InstanceVertexData, bone_offset)}, // bone_offset
};

/*----------------------------------------- Vertex Terrain ------------------------------------------*/

struct VertexTerrain
{
    vec2 pos;
    uint32_t heightmap_id;
};

inline const std::vector<VkVertexInputAttributeDescription> vertex_terrain_attr_desc
{
    {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexTerrain, pos)}, // pos
    {1, 0, VK_FORMAT_R32_UINT, offsetof(VertexTerrain, heightmap_id)}, // heightmap_id
};

#endif // VERTEX_H
