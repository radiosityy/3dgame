#ifndef TERRAIN_H
#define TERRAIN_H

#include "engine_3d.h"

class Terrain
{
public:
    Terrain(Engine3D& engine3d);
    Terrain(std::string_view filename);

    void draw(Engine3D& engine3d);

    float patchSizeX() const;
    float patchSizeZ() const;

    bool collision(const AABB&, float dh = 5) const;

#if EDITOR_ENABLE
    void serialize(std::string_view filename);

    bool rayIntersection(const Ray& ray, float& d) const;
    //std::optional<std::pair<PatchType, uint32_t>> pickPatch(const Ray& ray) const;
    void toolEdit(Engine3D& engine3d, const vec3& center, float radius, float dh);
#endif

private:
    void generatePatchVertices();

    float m_size_x = 100.0f;
    float m_size_z = 100.0f;
    float m_x = -0.5f * m_size_x;
    float m_z = -0.5f * m_size_z;
    uint32_t m_resolution_x = 2;
    uint32_t m_resolution_z = 2;
    float m_patch_size_x = m_size_x / static_cast<float>(m_resolution_x);
    float m_patch_size_z = m_size_z / static_cast<float>(m_resolution_z);

    struct VertexData
    {
        float height = 0.0f;
        uint32_t tex_id = 0;
//        uint32_t normal_map_id = 0;
    };

    std::vector<vec2> m_bounding_ys;

    std::vector<VertexData> m_vertex_data;

    std::vector<VertexTerrain> m_vertices;
    VertexBufferAllocation m_vb_alloc;
};

#endif // TERRAIN_H
