#include "terrain.h"
#include <iostream>

Terrain::Terrain(Engine3D& engine3d)
{
#if EDITOR_ENABLE
    const std::string filename = "terrain.dat";
    std::ifstream in(filename.data(), std::ios::binary);

    if(!in)
    {
        std::cout << "Failed to open file " << filename << ". Creating new terrain..." << std::endl;
        createNew(engine3d);
        return;
    }

    in.close();
#endif

    loadFromFile(engine3d);
}

#if EDITOR_ENABLE
void Terrain::createNew(Engine3D& engine3d)
{
    m_size_x = DEFAULT_SIZE_X;
    m_size_z = DEFAULT_SIZE_Z;
    m_x = -0.5f * m_size_x;
    m_z = -0.5f * m_size_z;
    m_resolution_x = DEFAULT_RES_X;
    m_resolution_z = DEFAULT_RES_Z;
    m_patch_size_x = m_size_x / static_cast<float>(m_resolution_x);
    m_patch_size_z = m_size_z / static_cast<float>(m_resolution_z);

    m_vertices.resize(m_resolution_x * m_resolution_z);
    m_patch_data.resize(m_vertices.size());
    m_bounding_ys.resize(m_vertices.size(), vec2(0.0f, 0.0f));
    m_vertex_data.resize((DEFAULT_PATCH_RES + 1) * (DEFAULT_PATCH_RES + 1) * m_resolution_x * m_resolution_z);

    for(uint32_t z = 0; z < m_resolution_z; z++)
    {
        for(uint32_t x = 0; x < m_resolution_x; x++)
        {
            const uint32_t vertex_id = z * m_resolution_x + x;
            const vec3 pos = vec3(m_x + x * m_patch_size_x, 0.0f, m_z + z * m_patch_size_z);

            m_vertices[vertex_id].pos = vec2(pos.x, pos.z);
            m_patch_data[vertex_id].center_pos_left = vec3(pos.x, 0.0f, pos.z + 0.5f * m_patch_size_z);
            m_patch_data[vertex_id].center_pos_top = vec3(pos.x + 0.5f * m_patch_size_x, 0.0f, pos.z + m_patch_size_z);
            m_patch_data[vertex_id].center_pos_right = vec3(pos.x + m_patch_size_x, 0.0f, pos.z + 0.5f * m_patch_size_z);
            m_patch_data[vertex_id].center_pos_bottom = vec3(pos.x + 0.5f * m_patch_size_x, 0.0f, pos.z);
            m_patch_data[vertex_id].center_pos = (m_patch_data[vertex_id].center_pos_bottom + m_patch_data[vertex_id].center_pos_right + m_patch_data[vertex_id].center_pos_left + m_patch_data[vertex_id].center_pos_top) / 4.0f;
            m_vertices[vertex_id].res = static_cast<float>(DEFAULT_PATCH_RES);
            m_patch_data[vertex_id].edge_res_left = static_cast<float>(DEFAULT_PATCH_RES);
            m_patch_data[vertex_id].edge_res_top = static_cast<float>(DEFAULT_PATCH_RES);
            m_patch_data[vertex_id].edge_res_right = static_cast<float>(DEFAULT_PATCH_RES);
            m_patch_data[vertex_id].edge_res_bottom = static_cast<float>(DEFAULT_PATCH_RES);
            m_vertices[vertex_id].vertex_offset = (DEFAULT_PATCH_RES + 1) * (DEFAULT_PATCH_RES + 1) * vertex_id;
        }
    }

    m_vb_alloc = engine3d.requestVertexBufferAllocation<VertexTerrain>(m_vertices.size());
    engine3d.updateVertexData(m_vb_alloc.vb, m_vb_alloc.data_offset, sizeof(VertexTerrain) * m_vertices.size(), m_vertices.data());

    engine3d.requestTerrainBufferAllocation(m_vertex_data.size() * sizeof(VertexData));
    engine3d.updateTerrainData(m_vertex_data.data(), 0, m_vertex_data.size() * sizeof(VertexData));
}
#endif

void Terrain::loadFromFile(Engine3D& engine3d)
{
    const std::string filename = "terrain.dat";
    std::ifstream in(filename.data(), std::ios::binary);

    if(!in)
    {
        throw std::runtime_error("Failed to open file " + filename + ".");
    }

    in.read(reinterpret_cast<char*>(&m_size_x), sizeof(m_size_x));
    in.read(reinterpret_cast<char*>(&m_size_z), sizeof(m_size_z));
    in.read(reinterpret_cast<char*>(&m_resolution_x), sizeof(m_resolution_x));
    in.read(reinterpret_cast<char*>(&m_resolution_z), sizeof(m_resolution_z));

    m_x = -0.5f * m_size_x;
    m_z = -0.5f * m_size_z;
    m_patch_size_x = m_size_x / static_cast<float>(m_resolution_x);
    m_patch_size_z = m_size_z / static_cast<float>(m_resolution_z);

    uint64_t bounding_ys_count = 0;
    in.read(reinterpret_cast<char*>(&bounding_ys_count), sizeof(uint64_t));
    m_bounding_ys.resize(bounding_ys_count);
    in.read(reinterpret_cast<char*>(m_bounding_ys.data()), m_bounding_ys.size() * sizeof(vec2));

    uint64_t patch_data_count = 0;
    in.read(reinterpret_cast<char*>(&patch_data_count), sizeof(uint64_t));
    m_patch_data.resize(patch_data_count);
    in.read(reinterpret_cast<char*>(m_patch_data.data()), m_patch_data.size() * sizeof(PatchData));

    uint64_t vertex_data_count = 0;
    in.read(reinterpret_cast<char*>(&vertex_data_count), sizeof(uint64_t));
    m_vertex_data.resize(vertex_data_count);
    in.read(reinterpret_cast<char*>(m_vertex_data.data()), m_vertex_data.size() * sizeof(VertexData));

    uint64_t vertex_count = 0;
    in.read(reinterpret_cast<char*>(&vertex_count), sizeof(uint64_t));
    m_vertices.resize(vertex_count);
    in.read(reinterpret_cast<char*>(m_vertices.data()), m_vertices.size() * sizeof(VertexTerrain));

    m_vb_alloc = engine3d.requestVertexBufferAllocation<VertexTerrain>(m_vertices.size());
    engine3d.updateVertexData(m_vb_alloc.vb, m_vb_alloc.data_offset, sizeof(VertexTerrain) * m_vertices.size(), m_vertices.data());

    engine3d.requestTerrainBufferAllocation(m_vertex_data.size() * sizeof(VertexData));
    engine3d.updateTerrainData(m_vertex_data.data(), 0, m_vertex_data.size() * sizeof(VertexData));
}

#if EDITOR_ENABLE
void Terrain::draw(Engine3D& engine3d, Camera& camera)
{
    for(uint32_t i = 0; i < m_vertices.size(); i++)
    {
        if(m_lod_enabled)
        {
            auto tessLevel = [&](vec3 center_pos, float max_tess_level)
            {
                const float d = distance(camera.pos(), center_pos);

                float tess_level = glm::clamp(max_tess_level * (50.0f / d), 2.0f, max_tess_level);
                tess_level = float(1u << static_cast<uint32_t>(std::log2(tess_level) + 0.5f));

                return tess_level;
            };

            m_vertices[i].edge_res_left = tessLevel(m_patch_data[i].center_pos_left, m_patch_data[i].edge_res_left);
            m_vertices[i].edge_res_top = tessLevel(m_patch_data[i].center_pos_top, m_patch_data[i].edge_res_top);
            m_vertices[i].edge_res_right = tessLevel(m_patch_data[i].center_pos_right, m_patch_data[i].edge_res_right);
            m_vertices[i].edge_res_bottom = tessLevel(m_patch_data[i].center_pos_bottom, m_patch_data[i].edge_res_bottom);
            m_vertices[i].lod_res = max(tessLevel(m_patch_data[i].center_pos, m_vertices[i].res), m_vertices[i].edge_res_left, m_vertices[i].edge_res_top, m_vertices[i].edge_res_right, m_vertices[i].edge_res_bottom);
        }
        else
        {
            m_vertices[i].edge_res_left = m_patch_data[i].edge_res_left;
            m_vertices[i].edge_res_top = m_patch_data[i].edge_res_top;
            m_vertices[i].edge_res_right = m_patch_data[i].edge_res_right;
            m_vertices[i].edge_res_bottom = m_patch_data[i].edge_res_bottom;
            m_vertices[i].lod_res = m_vertices[i].res;
        }

        const uint32_t vertex_count = m_vertices[i].lod_res * m_vertices[i].lod_res * 6;
        engine3d.draw(m_render_mode, m_vb_alloc.vb, m_vb_alloc.vertex_offset, vertex_count, i, {});
    }

    //TODO: optimize this
    engine3d.updateVertexData(m_vb_alloc.vb, m_vb_alloc.data_offset, sizeof(VertexTerrain) * m_vertices.size(), m_vertices.data());
}
#else
void Terrain::draw(Engine3D& engine3d, Camera& camera)
{
    for(uint32_t i = 0; i < m_vertices.size(); i++)
    {
        auto tessLevel = [&](vec3 center_pos, float max_tess_level)
        {
            const float d = distance(camera.pos(), center_pos);

            float tess_level = glm::clamp(max_tess_level * (50.0f / d), 2.0f, max_tess_level);
            tess_level = float(1u << static_cast<uint32_t>(std::log2(tess_level) + 0.5f));

            return tess_level;
        };

        m_vertices[i].edge_res_left = tessLevel(m_patch_data[i].center_pos_left, m_patch_data[i].edge_res_left);
        m_vertices[i].edge_res_top = tessLevel(m_patch_data[i].center_pos_top, m_patch_data[i].edge_res_top);
        m_vertices[i].edge_res_right = tessLevel(m_patch_data[i].center_pos_right, m_patch_data[i].edge_res_right);
        m_vertices[i].edge_res_bottom = tessLevel(m_patch_data[i].center_pos_bottom, m_patch_data[i].edge_res_bottom);
        m_vertices[i].lod_res = max(tessLevel(m_patch_data[i].center_pos, m_vertices[i].res), m_vertices[i].edge_res_left, m_vertices[i].edge_res_top, m_vertices[i].edge_res_right, m_vertices[i].edge_res_bottom);

        //TODO: optimize this
        engine3d.updateVertexData(m_vb_alloc.vb, m_vb_alloc.data_offset, sizeof(VertexTerrain) * m_vertices.size(), m_vertices.data());

        const uint32_t vertex_count = m_vertices[i].lod_res * m_vertices[i].lod_res * 6;
        engine3d.draw(RenderMode::Terrain, m_vb_alloc.vb, m_vb_alloc.vertex_offset, vertex_count, i, {}, {});
    }
}
#endif

float Terrain::patchSizeX() const
{
    return m_patch_size_x;
}

float Terrain::patchSizeZ() const
{
    return m_patch_size_z;
}

static float triHeightAtPoint(const vec3& t0, const vec3& t1, const vec3& t2, const vec2& p)
{
    const float denominator = (t1.z - t2.z) * (t0.x - t2.x) + (t2.x - t1.x) * (t0.z - t2.z);
    const float a = (p.x - t2.x);
    const float b = (p.y - t2.z);
    const float w0 = ((t1.z - t2.z) * a + (t2.x - t1.x) * b) / denominator;
    const float w1 = ((t2.z - t0.z) * a + (t0.x - t2.x) * b) / denominator;
    const float w2 = 1.0f - w0 - w1;

    return w0 * t0.y + w1 * t1.y + w2 * t2.y;
}

CollisionResult Terrain::collision(const AABB& aabb, float& dh_out) const
{
    const float min_x_coord = std::max<float>((aabb.min().x - m_x) / m_patch_size_x, 0);
    const float max_x_coord = std::min<float>((aabb.max().x - m_x) / m_patch_size_x, m_resolution_x);
    const float min_z_coord = std::max<float>((aabb.min().z - m_z) / m_patch_size_z, 0);
    const float max_z_coord = std::min<float>((aabb.max().z - m_z) / m_patch_size_z, m_resolution_z);

    const uint32_t min_patch_x = min_x_coord;
    const uint32_t max_patch_x = std::min<uint32_t>(max_x_coord, m_resolution_x - 1);
    const uint32_t min_patch_z = min_z_coord;
    const uint32_t max_patch_z = std::min<uint32_t>(max_z_coord, m_resolution_z - 1);

    const float min_vx = min_x_coord - static_cast<float>(min_patch_x);
    const float max_vx = max_x_coord - static_cast<float>(max_patch_x);
    const float min_vz = min_z_coord - static_cast<float>(min_patch_z);
    const float max_vz = max_z_coord - static_cast<float>(max_patch_z);

    float max_dh = std::numeric_limits<float>::lowest();

    auto height_cmp = [&](float h)
    {
        const float dh = h - aabb.min().y;
        if(dh > dh_out)
        {
            return true;
        }
        else if(dh > max_dh)
        {
            max_dh = dh;
        }
        return false;
    };

    for(uint32_t patch_z = min_patch_z; patch_z <= max_patch_z; patch_z++)
    {
        for(uint32_t patch_x = min_patch_x; patch_x <= max_patch_x; patch_x++)
        {
            const uint32_t patch_id = patch_z * m_resolution_x + patch_x;
            //TODO: is bounding_ys even worth using here?
            const vec2& bounding_ys = m_bounding_ys[patch_id];

//            if((bounding_ys[0] >= aabb.min().y) && (bounding_ys[1] <= aabb.max().y))
            {
                const uint32_t res_u32 = static_cast<uint32_t>(m_vertices[patch_id].res);

                const uint32_t min_patch_vx = (patch_x == static_cast<uint32_t>(min_patch_x)) ? static_cast<uint32_t>(m_vertices[patch_id].res * min_vx) : 0;
                const uint32_t min_patch_vz = (patch_z == static_cast<uint32_t>(min_patch_z)) ? static_cast<uint32_t>(m_vertices[patch_id].res * min_vz) : 0;
                const uint32_t max_patch_vx = (patch_x == static_cast<uint32_t>(max_patch_x)) ? std::min<uint32_t>(res_u32, static_cast<uint32_t>(m_vertices[patch_id].res * max_vx) + 1) : res_u32;
                const uint32_t max_patch_vz = (patch_z == static_cast<uint32_t>(max_patch_z)) ? std::min<uint32_t>(res_u32, static_cast<uint32_t>(m_vertices[patch_id].res * max_vz) + 1) : res_u32;

                /*test the vertices inside the bounding volume*/
                if((max_patch_vz - min_patch_vz) > 4 && (max_patch_vx - min_patch_vx) > 4)
                {
                    for(uint32_t vz = min_patch_vz + 2; vz <= max_patch_vz - 2; vz++)
                    {
                        for(uint32_t vx = min_patch_vx + 2; vx <= max_patch_vx - 2; vx++)
                        {
                            const float vy = m_vertex_data[m_vertices[patch_id].vertex_offset + vz * (res_u32 + 1) + vx].height;
                            if(height_cmp(vy)) return CollisionResult::Collision;
                        }
                    }
                }

                /*test the outer triangles*/
                const vec2 aabb_v0(aabb.min().x, aabb.min().z);
                const vec2 aabb_v1(aabb.min().x, aabb.max().z);
                const vec2 aabb_v2(aabb.max().x, aabb.max().z);
                const vec2 aabb_v3(aabb.max().x, aabb.min().z);

                auto right_tri = [&](const vec3& t0, const vec3& t1, const vec3& t2)
                {
                    const float tan_z = (t1.z - t2.z) / (t1.x - t0.x);
                    const float tan_x = 1.0f / tan_z;

                    //top-right vertex
                    const vec2 v2(std::min<float>(aabb_v2.x, t1.x), std::min<float>(aabb_v2.y, t1.z));
                    const float h = triHeightAtPoint(t0, t1, t2, v2);
                    if(height_cmp(h)) return true;

                    //bottom-right vertex
                    const float z = t2.z + tan_z * (t2.x - v2.x);

                    //if vertex outside the triangle, find its intersection point with the triangle
                    if(aabb_v3.y < z)
                    {
                        const vec2 v3(v2.x, z);
                        const float h = triHeightAtPoint(t0, t1, t2, v3);
                        if(height_cmp(h)) return true;
                    }
                    else
                    {
                        //if vertex inside the triangle, take it as it is
                        const vec2 v3(v2.x, aabb_v3.y);
                        float h = triHeightAtPoint(t0, t1, t2, v3);
                        if(height_cmp(h)) return true;

                        //then take the intersection of the triangle diagonal edge with rectangle's v0-v3 edge
                        const float x = t2.x - tan_x * (v3.y - t2.z);
                        const vec2 v0(x, v3.y);
                        h = triHeightAtPoint(t0, t1, t2, v0);
                        if(height_cmp(h)) return true;
                    }

                    //top_left vertex
                    const float x = t2.x - tan_x * (v2.y - t2.z);

                    //if vertex outside the triangle, find its intersection point with the triangle
                    if(aabb_v1.x < x)
                    {
                        const vec2 v1(x, v2.y);
                        const float h = triHeightAtPoint(t0, t1, t2, v1);
                        if(height_cmp(h)) return true;
                    }
                    else
                    {
                        //if vertex inside the triangle, take it as it is
                        const vec2 v1(aabb_v1.x, v2.y);
                        float h = triHeightAtPoint(t0, t1, t2, v1);
                        if(height_cmp(h)) return true;

                        //then take the intersection of the triangle diagonal edge with rectangle's v0-v1 edge
                        const float z = t2.z + tan_z * (t2.x - v1.x);
                        const vec2 v0(v1.x, z);
                        h = triHeightAtPoint(t0, t1, t2, v0);
                        if(height_cmp(h)) return true;
                    }

                    return false;
                };

                auto left_tri = [&](const vec3& t0, const vec3& t1, const vec3& t2)
                {
                    const float tan_z = (t0.z - t1.z) / (t2.x - t1.x);
                    const float tan_x = 1.0f / tan_z;

                    //bottom-left vertex
                    const vec2 v0(std::max<float>(aabb_v0.x, t1.x), std::max<float>(aabb_v0.y, t1.z));
                    const float h = triHeightAtPoint(t0, t1, t2, v0);
                    if(height_cmp(h)) return true;

                    //top-left vertex
                    const float z = t1.z + tan_z * (t2.x - v0.x);

                    //if vertex outside the triangle, find its intersection point with the triangle
                    if(aabb_v1.y > z)
                    {
                        const vec2 v1(v0.x, z);
                        const float h = triHeightAtPoint(t0, t1, t2, v1);
                        if(height_cmp(h)) return true;
                    }
                    else
                    {
                        //if vertex inside the triangle, take it as it is
                        const vec2 v1(v0.x, aabb_v1.y);
                        float h = triHeightAtPoint(t0, t1, t2, v1);
                        if(height_cmp(h)) return true;

                        //then take the intersection of the triangle diagonal edge with rectangle's v0-v1 edge
                        const float x = t2.x - tan_x * (v1.y - t2.z);
                        const vec2 v2(x, v1.y);
                        h = triHeightAtPoint(t0, t1, t2, v2);
                        if(height_cmp(h)) return true;
                    }

                    //bottom_right vertex
                    const float x = t2.x - tan_x * (v0.y - t2.z);

                    //if vertex outside the triangle, find its intersection point with the triangle
                    if(aabb_v3.x > x)
                    {
                        const vec2 v3(x, v0.y);
                        const float h = triHeightAtPoint(t0, t1, t2, v3);
                        if(height_cmp(h)) return true;
                    }
                    else
                    {
                        //if vertex inside the triangle, take it as it is
                        const vec2 v3(aabb_v3.x, v0.y);
                        float h = triHeightAtPoint(t0, t1, t2, v3);
                        if(height_cmp(h)) return true;

                        //then take the intersection of the triangle diagonal edge with rectangle's v0-v3 edge
                        const float z = t2.z + tan_z * (t2.x - v3.x);
                        const vec2 v2(v3.x, z);
                        h = triHeightAtPoint(t0, t1, t2, v2);
                        if(height_cmp(h)) return true;
                    }

                    return false;
                };

                /*test triangles*/
                const float dx = m_patch_size_x / m_vertices[patch_id].res;
                const float dz = m_patch_size_z / m_vertices[patch_id].res;

                auto test_triangles = [&](float vx, float vz)
                {
                    const uint32_t base_vid = m_vertices[patch_id].vertex_offset + vz * (res_u32 + 1) + vx;
                    const vec3 v0(m_x + m_patch_size_x * patch_x + dx * vx, m_vertex_data[base_vid].height, m_z + m_patch_size_z * patch_z + dz * vz);
                    const vec3 v1(m_x + m_patch_size_x * patch_x + dx * vx, m_vertex_data[base_vid + res_u32 + 1].height, m_z + m_patch_size_z * patch_z + dz * (vz + 1));
                    const vec3 v2(m_x + m_patch_size_x * patch_x + dx * (vx + 1), m_vertex_data[base_vid + res_u32 + 2].height, m_z + m_patch_size_z * patch_z + dz * (vz + 1));
                    const vec3 v3(m_x + m_patch_size_x * patch_x + dx * (vx + 1), m_vertex_data[base_vid + 1].height, m_z + m_patch_size_z * patch_z + dz * vz);

                    if(left_tri(v1, v0, v3)) return true;
                    if(right_tri(v1, v2, v3)) return true;
                    return false;
                };

                //bottom row
                for(uint32_t vx = min_patch_vx; vx < max_patch_vx; vx++)
                {
                    if(test_triangles(vx, min_patch_vz)) return CollisionResult::Collision;
                }

                //internal rows
                for(uint32_t vz = min_patch_vz + 1; vz < max_patch_vz - 1; vz++)
                {
                    if(test_triangles(min_patch_vx, vz)) return CollisionResult::Collision;
                    if(test_triangles(max_patch_vx - 1, vz)) return CollisionResult::Collision;
                }

                //top row
                for(uint32_t vx = min_patch_vx; vx < max_patch_vx; vx++)
                {
                    if(test_triangles(vx, max_patch_vz - 1)) return CollisionResult::Collision;
                }
            }
        }
    }

    if(max_dh >= -dh_out)
    {
        dh_out = max_dh;
        return CollisionResult::ResolvedCollision;
    }

    return CollisionResult::Airborne;
}

#if EDITOR_ENABLE

void Terrain::setSizeX(Engine3D& engine3d, float size_x)
{
    m_size_x = size_x;
    m_x = -0.5f * m_size_x;
    m_patch_size_x = m_size_x / static_cast<float>(m_resolution_x);

    for(uint32_t z = 0; z < m_resolution_z; z++)
    {
        for(uint32_t x = 0; x < m_resolution_x; x++)
        {
            const uint32_t vertex_id = z * m_resolution_x + x;
            const vec3 pos = vec3(m_x + x * m_patch_size_x, 0.0f, m_z + z * m_patch_size_z);

            m_vertices[vertex_id].pos = vec2(pos.x, pos.z);
            m_patch_data[vertex_id].center_pos_left = vec3(pos.x, 0.0f, pos.z + 0.5f * m_patch_size_z);
            m_patch_data[vertex_id].center_pos_top = vec3(pos.x + 0.5f * m_patch_size_x, 0.0f, pos.z + m_patch_size_z);
            m_patch_data[vertex_id].center_pos_right = vec3(pos.x + m_patch_size_x, 0.0f, pos.z + 0.5f * m_patch_size_z);
            m_patch_data[vertex_id].center_pos_bottom = vec3(pos.x + 0.5f * m_patch_size_x, 0.0f, pos.z);
            m_patch_data[vertex_id].center_pos = (m_patch_data[vertex_id].center_pos_bottom + m_patch_data[vertex_id].center_pos_right + m_patch_data[vertex_id].center_pos_left + m_patch_data[vertex_id].center_pos_top) / 4.0f;
        }
    }

    engine3d.updateVertexData(m_vb_alloc.vb, m_vb_alloc.data_offset, sizeof(VertexTerrain) * m_vertices.size(), m_vertices.data());
}

void Terrain::setSizeZ(Engine3D& engine3d, float size_z)
{
    m_size_z = size_z;
    m_z = -0.5f * m_size_z;
    m_patch_size_z = m_size_z / static_cast<float>(m_resolution_z);

    for(uint32_t z = 0; z < m_resolution_z; z++)
    {
        for(uint32_t x = 0; x < m_resolution_x; x++)
        {
            const uint32_t vertex_id = z * m_resolution_x + x;
            const vec3 pos = vec3(m_x + x * m_patch_size_x, 0.0f, m_z + z * m_patch_size_z);

            m_vertices[vertex_id].pos = vec2(pos.x, pos.z);
            m_patch_data[vertex_id].center_pos_left = vec3(pos.x, 0.0f, pos.z + 0.5f * m_patch_size_z);
            m_patch_data[vertex_id].center_pos_top = vec3(pos.x + 0.5f * m_patch_size_x, 0.0f, pos.z + m_patch_size_z);
            m_patch_data[vertex_id].center_pos_right = vec3(pos.x + m_patch_size_x, 0.0f, pos.z + 0.5f * m_patch_size_z);
            m_patch_data[vertex_id].center_pos_bottom = vec3(pos.x + 0.5f * m_patch_size_x, 0.0f, pos.z);
            m_patch_data[vertex_id].center_pos = (m_patch_data[vertex_id].center_pos_bottom + m_patch_data[vertex_id].center_pos_right + m_patch_data[vertex_id].center_pos_left + m_patch_data[vertex_id].center_pos_top) / 4.0f;
        }
    }

    engine3d.updateVertexData(m_vb_alloc.vb, m_vb_alloc.data_offset, sizeof(VertexTerrain) * m_vertices.size(), m_vertices.data());
}

void Terrain::serialize(std::string_view filename)
{
    std::ofstream out(filename.data(), std::ios::binary);

    out.write(reinterpret_cast<const char*>(&m_size_x), sizeof(m_size_x));
    out.write(reinterpret_cast<const char*>(&m_size_z), sizeof(m_size_z));
    out.write(reinterpret_cast<const char*>(&m_resolution_x), sizeof(m_resolution_x));
    out.write(reinterpret_cast<const char*>(&m_resolution_z), sizeof(m_resolution_z));

    const uint64_t bounding_ys_count = m_bounding_ys.size();
    out.write(reinterpret_cast<const char*>(&bounding_ys_count), sizeof(uint64_t));
    out.write(reinterpret_cast<const char*>(m_bounding_ys.data()), m_bounding_ys.size() * sizeof(vec2));
    const uint64_t patch_data_count = m_patch_data.size();
    out.write(reinterpret_cast<const char*>(&patch_data_count), sizeof(uint64_t));
    out.write(reinterpret_cast<const char*>(m_patch_data.data()), m_patch_data.size() * sizeof(PatchData));
    const uint64_t vertex_data_count = m_vertex_data.size();
    out.write(reinterpret_cast<const char*>(&vertex_data_count), sizeof(uint64_t));
    out.write(reinterpret_cast<const char*>(m_vertex_data.data()), m_vertex_data.size() * sizeof(VertexData));
    const uint64_t vertex_count = m_vertices.size();
    out.write(reinterpret_cast<const char*>(&vertex_count), sizeof(uint64_t));
    out.write(reinterpret_cast<const char*>(m_vertices.data()), m_vertices.size() * sizeof(VertexTerrain));
}

bool Terrain::rayIntersection(const Ray& ray, float& d) const
{
    d = std::numeric_limits<float>::max();
    bool intersection_found = false;

    for(uint32_t patch_z = 0; patch_z < m_resolution_z; patch_z++)
    {
        for(uint32_t patch_x = 0; patch_x < m_resolution_x; patch_x++)
        {
            const uint32_t patch_id = patch_z * m_resolution_x + patch_x;
            const AABB aabb(vec3(m_x + m_patch_size_x * patch_x, m_bounding_ys[patch_id][0], m_z + m_patch_size_z * patch_z), vec3(m_x + m_patch_size_x * (patch_x + 1.0f), m_bounding_ys[patch_id][1], m_z + m_patch_size_z * (patch_z + 1.0f)));

            if(intersect(ray, aabb))
            {
                const uint32_t res_u32 = static_cast<uint32_t>(m_vertices[patch_id].res);
                const float dx = m_patch_size_x / m_vertices[patch_id].res;
                const float dz = m_patch_size_z / m_vertices[patch_id].res;

                for(uint32_t x = 0; x < static_cast<uint32_t>(m_vertices[patch_id].res); x++)
                {
                    for(uint32_t z = 0; z < static_cast<uint32_t>(m_vertices[patch_id].res); z++)
                    {
                        const vec3 v0 = vec3(m_x + m_patch_size_x * patch_x + dx * x,
                                             m_vertex_data[(res_u32 + 1) * (res_u32 + 1) * patch_id + z * (res_u32 + 1) + x].height,
                                             m_z + m_patch_size_z * patch_z + dz * z);
                        const vec3 v1 = vec3(m_x + m_patch_size_x * patch_x + dx * (x + 1),
                                             m_vertex_data[(res_u32 + 1) * (res_u32 + 1) * patch_id + z * (res_u32 + 1) + (x + 1)].height,
                                             m_z + m_patch_size_z * patch_z + dz * z);
                        const vec3 v2 = vec3(m_x + m_patch_size_x * patch_x + dx * x,
                                             m_vertex_data[(res_u32 + 1) * (res_u32 + 1) * patch_id + (z + 1) * (res_u32 + 1) + x].height,
                                             m_z + m_patch_size_z * patch_z + dz * (z + 1));
                        const vec3 v3 = vec3(m_x + m_patch_size_x * patch_x + dx * (x + 1),
                                             m_vertex_data[(res_u32 + 1) * (res_u32 + 1) * patch_id + (z + 1) * (res_u32 + 1) + (x + 1)].height,
                                             m_z + m_patch_size_z * patch_z + dz * (z + 1));

                        float temp_d;

                        if(intersect(ray, v0, v1, v2, temp_d))
                        {
                            if(temp_d < d)
                            {
                                d = temp_d;
                            }

                            intersection_found = true;
                        }

                        if(intersect(ray, v1, v2, v3, temp_d))
                        {
                            if(temp_d < d)
                            {
                                d = temp_d;
                            }

                            intersection_found = true;
                        }
                    }
                }
            }
        }
    }

    return intersection_found;
}

void Terrain::toolEdit(Engine3D& engine3d, const vec3& center, float radius, float dh)
{
    const float min_x_coord = std::max<float>((center.x - radius - m_x) / m_patch_size_x, 0);
    const float max_x_coord = std::min<float>((center.x + radius - m_x) / m_patch_size_x, m_resolution_x);
    const float min_z_coord = std::max<float>((center.z - radius - m_z) / m_patch_size_z, 0);
    const float max_z_coord = std::min<float>((center.z + radius - m_z) / m_patch_size_z, m_resolution_z);

    const uint32_t min_patch_x = min_x_coord;
    const uint32_t max_patch_x = std::min<uint32_t>(max_x_coord, m_resolution_x - 1);
    const uint32_t min_patch_z = min_z_coord;
    const uint32_t max_patch_z = std::min<uint32_t>(max_z_coord, m_resolution_z - 1);;

    const float min_vx = min_x_coord - static_cast<float>(min_patch_x);
    const float max_vx = max_x_coord - static_cast<float>(max_patch_x);
    const float min_vz = min_z_coord - static_cast<float>(min_patch_z);
    const float max_vz = max_z_coord - static_cast<float>(max_patch_z);

    for(uint32_t patch_z = min_patch_z; patch_z <= max_patch_z; patch_z++)
    {
        for(uint32_t patch_x = min_patch_x; patch_x <= max_patch_x; patch_x++)
        {
            const uint32_t patch_id = patch_z * m_resolution_x + patch_x;

            const uint32_t res_u32 = static_cast<uint32_t>(m_vertices[patch_id].res);

            const uint32_t min_patch_vx = (patch_x == min_patch_x) ? static_cast<uint32_t>(m_vertices[patch_id].res * min_vx) : 0;
            const uint32_t min_patch_vz = (patch_z == min_patch_z) ? static_cast<uint32_t>(m_vertices[patch_id].res * min_vz) : 0;
            const uint32_t max_patch_vx = (patch_x == max_patch_x) ? std::min<uint32_t>(res_u32, static_cast<uint32_t>(m_vertices[patch_id].res * max_vx) + 1) : res_u32;
            const uint32_t max_patch_vz = (patch_z == max_patch_z) ? std::min<uint32_t>(res_u32, static_cast<uint32_t>(m_vertices[patch_id].res * max_vz) + 1) : res_u32;

            for(uint32_t vz = min_patch_vz; vz <= max_patch_vz; vz++)
            {
                for(uint32_t vx = min_patch_vx; vx <= max_patch_vx; vx++)
                {
                    const vec3 v(m_x + m_patch_size_x * (patch_x + vx / m_vertices[patch_id].res),
                                 m_vertex_data[(res_u32 + 1) * (res_u32 + 1) * patch_id + vz * (res_u32 + 1) + vx].height,
                                 m_z + m_patch_size_z * (patch_z + vz / m_vertices[patch_id].res));

                    if(glm::distance(center, v) <= radius)
                    {
                        m_vertex_data[(res_u32 + 1) * (res_u32 + 1) * patch_id + vz * (res_u32 + 1) + vx].height += dh;

                        m_bounding_ys[patch_id][0] = std::min(m_bounding_ys[patch_id][0], m_vertex_data[(res_u32 + 1) * (res_u32 + 1) * patch_id + vz * (res_u32 + 1) + vx].height);
                        m_bounding_ys[patch_id][1] = std::max(m_bounding_ys[patch_id][1], m_vertex_data[(res_u32 + 1) * (res_u32 + 1) * patch_id + vz * (res_u32 + 1) + vx].height);
                    }
                }
            }
        }
    }

    //TODO: only update the data that's changed
    engine3d.updateTerrainData(m_vertex_data.data(), 0, m_vertex_data.size() * sizeof(VertexData));
}

void Terrain::toggleWireframe()
{
    if(RenderMode::Terrain == m_render_mode)
    {
        m_render_mode = RenderMode::TerrainWireframe;
    }
    else
    {
        m_render_mode = RenderMode::Terrain;
    }
}

void Terrain::toggleLod()
{
    m_lod_enabled = !m_lod_enabled;
}
#endif
