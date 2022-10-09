#ifndef TERRAIN_H
#define TERRAIN_H

#include "engine_3d.h"
#include "collision.h"
#include "vertex.h"

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

    void draw(Engine3D& engine3d, Camera& camera);

    float patchSizeX() const;
    float patchSizeZ() const;

    CollisionResult collision(const AABB&, float& dh) const;

#if EDITOR_ENABLE
    void serialize(std::string_view filename);

    bool rayIntersection(const Ray& ray, float& d) const;
    //std::optional<std::pair<PatchType, uint32_t>> pickPatch(const Ray& ray) const;
    void toolEdit(Engine3D& engine3d, const vec3& center, float radius, float dh);

    void setSizeX(Engine3D& engine3d, float size_x);
    void setSizeZ(Engine3D& engine3d, float size_z);

    void toggleWireframe();
    void toggleLod();
#endif

private:
#if EDITOR_ENABLE
    static constexpr float DEFAULT_SIZE_X = 100.0f;
    static constexpr float DEFAULT_SIZE_Z = 100.0f;
    static constexpr uint32_t DEFAULT_RES_X = 2;
    static constexpr uint32_t DEFAULT_RES_Z = 2;
    static constexpr uint32_t DEFAULT_PATCH_RES = 64;

    void createNew(Engine3D&);
#endif
    void loadFromFile(Engine3D&);

    float m_size_x;
    float m_size_z;
    float m_x;
    float m_z;
    uint32_t m_resolution_x;
    uint32_t m_resolution_z;
    float m_patch_size_x;
    float m_patch_size_z;

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

#if EDITOR_ENABLE
    RenderMode m_render_mode = RenderMode::Terrain;
    bool m_lod_enabled = true;
#endif
};

#endif // TERRAIN_H
