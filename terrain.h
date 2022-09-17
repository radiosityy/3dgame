#ifndef TERRAIN_H
#define TERRAIN_H

#include "engine_3d.h"

enum class CollisionResult
{
    Collision,
    ResolvedCollision,
    Airborne
};

class Terrain
{
public:
    Terrain(Engine3D& engine3d);
    Terrain(std::string_view filename);

    void draw(Engine3D& engine3d, Camera& camera);

    float patchSizeX() const;
    float patchSizeZ() const;

    CollisionResult collision(const AABB&, float& dh) const;

#if EDITOR_ENABLE
    void serialize(std::string_view filename);

    bool rayIntersection(const Ray& ray, float& d) const;
    //std::optional<std::pair<PatchType, uint32_t>> pickPatch(const Ray& ray) const;
    void toolEdit(Engine3D& engine3d, const vec3& center, float radius, float dh);
#endif

private:
    void generatePatchVertices();

    float m_size_x = 1000.0f;
    float m_size_z = 1000.0f;
    float m_x = -0.5f * m_size_x;
    float m_z = -0.5f * m_size_z;
    uint32_t m_resolution_x = 10;
    uint32_t m_resolution_z = 10;
    float m_patch_size_x = m_size_x / static_cast<float>(m_resolution_x);
    float m_patch_size_z = m_size_z / static_cast<float>(m_resolution_z);

    struct VertexData
    {
        float height = 0.0f;
        uint32_t tex_id = 0;
    };

    struct PatchData
    {
        vec3 center_pos_left;
        vec3 center_pos_top;
        vec3 center_pos_right;
        vec3 center_pos_bottom;
        vec3 center_pos;
        float edge_res_left;
        float edge_res_top;
        float edge_res_right;
        float edge_res_bottom;
    };

    std::vector<vec2> m_bounding_ys;

    std::vector<PatchData> m_patch_data;
    std::vector<VertexData> m_vertex_data;

    std::vector<VertexTerrain> m_vertices;
    VertexBufferAllocation m_vb_alloc;
};

#endif // TERRAIN_H
