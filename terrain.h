#ifndef TERRAIN_H
#define TERRAIN_H

#include "renderer.h"
#include "collision.h"
#include "vertex.h"

class Terrain
{
public:
    Terrain(Renderer& renderer);

    void draw(Renderer& renderer);

    float patchSize() const;

    float collision(const AABB&, float max_dh) const;

#if EDITOR_ENABLE
    void saveToFile();

    bool rayIntersection(const Ray& ray, float& d) const;
    //std::optional<std::pair<PatchType, uint32_t>> pickPatch(const Ray& ray) const;
    void toolEdit(Renderer& renderer, const vec3& center, float radius, float dh);

    void setSize(Renderer& renderer, float size);

    void toggleWireframe();
    void toggleLod();
#endif

private:
    static const inline auto TERRAIN_FILENAME = "terrain.dat";
#if EDITOR_ENABLE
    static const inline float DEFAULT_SIZE = 100.0f;
    static const inline uint32_t DEFAULT_PATCH_COUNT = 2;

    void createNew();
#endif
    static constexpr uint32_t TOTAL_PATCH_VERTEX_COUNT = (MAX_TESS_LEVEL + 1) * (MAX_TESS_LEVEL + 1);
    void calcXYFromSize() noexcept;
    void loadFromFile(Renderer&);

    float m_size;
    float m_x;
    float m_z;
    uint32_t m_patch_count;
    float m_patch_size;

    struct VertexData
    {
        uint32_t tex_id = 0;
    };

    std::vector<vec2> m_bounding_ys;

    std::vector<std::array<float, TOTAL_PATCH_VERTEX_COUNT>> m_heightmaps;
    std::vector<VertexData> m_vertex_data;

    std::vector<VertexTerrain> m_patch_vertices;
    VertexBufferAllocation m_vb_alloc;
    RenderMode m_render_mode = RenderMode::Terrain;

#if EDITOR_ENABLE
    bool m_lod_enabled = true;
#endif
};

#endif // TERRAIN_H
