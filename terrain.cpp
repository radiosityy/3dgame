#include "terrain.h"
#include <iostream>

Terrain::Terrain(Engine3D& engine3d)
{
    m_vertices.resize(m_resolution_x * m_resolution_z);
    m_bounding_ys.resize(m_vertices.size(), vec2(0.0f, 0.0f));
    generatePatchVertices();

    m_vertex_data.resize(65 * 65 * m_resolution_x * m_resolution_z);

    m_vb_alloc = engine3d.requestVertexBufferAllocation<VertexTerrain>(m_vertices.size());
    engine3d.updateVertexData(m_vb_alloc.vb, m_vb_alloc.data_offset, sizeof(VertexTerrain) * m_vertices.size(), m_vertices.data());

    engine3d.requestTerrainBufferAllocation(m_vertex_data.size() * sizeof(VertexData));
    engine3d.updateTerrainData(m_vertex_data.data(), 0, m_vertex_data.size() * sizeof(VertexData));
}

Terrain::Terrain(std::string_view filename)
{
    std::ifstream in(filename.data(), std::ios::binary);
}

void Terrain::generatePatchVertices()
{
    for(uint32_t z = 0; z < m_resolution_z; z++)
    {
        for(uint32_t x = 0; x < m_resolution_x; x++)
        {
            const uint32_t vertex_id = z * m_resolution_x + x;
            const vec3 pos = vec3(m_x + x * m_patch_size_x, 0.0f, m_z + z * m_patch_size_z);

            m_vertices[vertex_id].pos = pos;
            m_vertices[vertex_id].center_pos_left = vec3(pos.x, 0.0f, pos.z + 0.5f * m_patch_size_z);
            m_vertices[vertex_id].center_pos_bottom = vec3(pos.x + 0.5f * m_patch_size_x, 0.0f, pos.z);
            m_vertices[vertex_id].center_pos_right = vec3(pos.x + m_patch_size_x, 0.0f, pos.z + 0.5f * m_patch_size_z);
            m_vertices[vertex_id].center_pos_top = vec3(pos.x + 0.5f * m_patch_size_x, 0.0f, pos.z + m_patch_size_z);
            m_vertices[vertex_id].tess_level = 64.0f;
            m_vertices[vertex_id].outer_tess_level_0 = 64.0f;
            m_vertices[vertex_id].outer_tess_level_1 = 64.0f;
            m_vertices[vertex_id].outer_tess_level_2 = 64.0f;
            m_vertices[vertex_id].outer_tess_level_3 = 64.0f;
            m_vertices[vertex_id].vertex_offset = 65 * 65 * vertex_id;
        }
    }
}

void Terrain::draw(Engine3D& engine3d)
{
    engine3d.draw(RenderMode::Terrain, m_vb_alloc.vb, m_vb_alloc.vertex_offset, m_vertices.size(), 0, {}, {});
}

float Terrain::patchSizeX() const
{
    return m_patch_size_x;
}

float Terrain::patchSizeZ() const
{
    return m_patch_size_z;
}

bool Terrain::collision(const AABB& aabb, float dh) const
{
    const float min_x_coord = std::max<float>((aabb.min().x - m_x) / m_patch_size_x, 0);
    const float max_x_coord = std::min<float>((aabb.max().x - m_x) / m_patch_size_x, m_resolution_x);
    const float min_z_coord = std::max<float>((aabb.min().z - m_z) / m_patch_size_z, 0);
    const float max_z_coord = std::min<float>((aabb.max().z - m_z) / m_patch_size_z, m_resolution_z);

    const uint32_t min_patch_x = min_x_coord;
    const uint32_t max_patch_x = std::min<uint32_t>(max_x_coord, m_resolution_x - 1);
    const uint32_t min_patch_z = min_z_coord;
    const uint32_t max_patch_z = std::min<uint32_t>(max_z_coord, m_resolution_z - 1);;

    const float min_vx = min_x_coord - static_cast<float>(min_patch_x);
    const float max_vx = max_x_coord - static_cast<float>(max_patch_x);
    const float min_vz = min_z_coord - static_cast<float>(min_patch_z);
    const float max_vz = max_z_coord - static_cast<float>(max_patch_z);

    float max_h = std::numeric_limits<float>::lowest();

    for(uint32_t patch_z = min_patch_z; patch_z <= max_patch_z; patch_z++)
    {
        for(uint32_t patch_x = min_patch_x; patch_x <= max_patch_x; patch_x++)
        {
            const uint32_t patch_id = patch_z * m_resolution_x + patch_x;
            //TODO: is bounding_ys even worth using here?
            const vec2& bounding_ys = m_bounding_ys[patch_id];

//            if((bounding_ys[0] >= aabb.min().y) && (bounding_ys[1] <= aabb.max().y))
            {
                const uint32_t tess_level_u32 = static_cast<uint32_t>(m_vertices[patch_id].tess_level);

                const uint32_t min_patch_vx = (patch_x == static_cast<uint32_t>(min_patch_x)) ? static_cast<uint32_t>(m_vertices[patch_id].tess_level * min_vx) : 0;
                const uint32_t min_patch_vz = (patch_z == static_cast<uint32_t>(min_patch_z)) ? static_cast<uint32_t>(m_vertices[patch_id].tess_level * min_vz) : 0;
                const uint32_t max_patch_vx = (patch_x == static_cast<uint32_t>(max_patch_x)) ? std::min<uint32_t>(tess_level_u32, static_cast<uint32_t>(m_vertices[patch_id].tess_level * max_vx) + 1) : tess_level_u32;
                const uint32_t max_patch_vz = (patch_z == static_cast<uint32_t>(max_patch_z)) ? std::min<uint32_t>(tess_level_u32, static_cast<uint32_t>(m_vertices[patch_id].tess_level * max_vz) + 1) : tess_level_u32;

                for(uint32_t vz = min_patch_vz + 1; vz <= max_patch_vz - 1; vz++)
                {
                    for(uint32_t vx = min_patch_vx + 1; vx <= max_patch_vx - 1; vx++)
                    {
                        const float vy = m_vertex_data[(tess_level_u32 + 1) * (tess_level_u32 + 1) * patch_id + vz * (tess_level_u32 + 1) + vx].height;

                        if(std::abs(vy - aabb.min().y) > dh)
                        {
                            return false;
                        }
                        else if(vy > max_h)
                        {
                            max_h = vy;
                        }
                    }
                }


            }
        }
    }

    return false;
}

#if EDITOR_ENABLE

void Terrain::serialize(std::string_view filename)
{
    std::ofstream out(filename.data(), std::ios::binary);

    out.write(reinterpret_cast<const char*>(&m_x), sizeof(m_x));
    out.write(reinterpret_cast<const char*>(&m_z), sizeof(m_z));
    out.write(reinterpret_cast<const char*>(&m_size_x), sizeof(m_size_x));
    out.write(reinterpret_cast<const char*>(&m_size_z), sizeof(m_size_z));
    out.write(reinterpret_cast<const char*>(&m_resolution_x), sizeof(m_resolution_x));
    out.write(reinterpret_cast<const char*>(&m_resolution_z), sizeof(m_resolution_z));
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
                const uint32_t tess_level_u32 = static_cast<uint32_t>(m_vertices[patch_id].tess_level);
                const float dx = m_patch_size_x / m_vertices[patch_id].tess_level;
                const float dz = m_patch_size_z / m_vertices[patch_id].tess_level;

                for(uint32_t x = 0; x < static_cast<uint32_t>(m_vertices[patch_id].tess_level); x++)
                {
                    for(uint32_t z = 0; z < static_cast<uint32_t>(m_vertices[patch_id].tess_level); z++)
                    {
                        const vec3 v0 = vec3(m_x + m_patch_size_x * patch_x + dx * x,
                                             m_vertex_data[(tess_level_u32 + 1) * (tess_level_u32 + 1) * patch_id + z * (tess_level_u32 + 1) + x].height,
                                             m_z + m_patch_size_z * patch_z + dz * z);
                        const vec3 v1 = vec3(m_x + m_patch_size_x * patch_x + dx * x,
                                             m_vertex_data[(tess_level_u32 + 1) * (tess_level_u32 + 1) * patch_id + (z + 1) * (tess_level_u32 + 1) + x].height,
                                             m_z + m_patch_size_z * patch_z + dz * (z + 1));
                        const vec3 v2 = vec3(m_x + m_patch_size_x * patch_x + dx * (x + 1),
                                             m_vertex_data[(tess_level_u32 + 1) * (tess_level_u32 + 1) * patch_id + z * (tess_level_u32 + 1) + (x + 1)].height,
                                             m_z + m_patch_size_z * patch_z + dz * z);
                        const vec3 v3 = vec3(m_x + m_patch_size_x * patch_x + dx * (x + 1),
                                             m_vertex_data[(tess_level_u32 + 1) * (tess_level_u32 + 1) * patch_id + (z + 1) * (tess_level_u32 + 1) + (x + 1)].height,
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

                        if(intersect(ray, v1, v3, v2, temp_d))
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

            const uint32_t tess_level_u32 = static_cast<uint32_t>(m_vertices[patch_id].tess_level);

            const uint32_t min_patch_vx = (patch_x == min_patch_x) ? static_cast<uint32_t>(m_vertices[patch_id].tess_level * min_vx) : 0;
            const uint32_t min_patch_vz = (patch_z == min_patch_z) ? static_cast<uint32_t>(m_vertices[patch_id].tess_level * min_vz) : 0;
            const uint32_t max_patch_vx = (patch_x == max_patch_x) ? std::min<uint32_t>(tess_level_u32, static_cast<uint32_t>(m_vertices[patch_id].tess_level * max_vx) + 1) : tess_level_u32;
            const uint32_t max_patch_vz = (patch_z == max_patch_z) ? std::min<uint32_t>(tess_level_u32, static_cast<uint32_t>(m_vertices[patch_id].tess_level * max_vz) + 1) : tess_level_u32;

            std::cout << min_patch_vx << " " << max_patch_vx << " " << min_patch_vz << " " << max_patch_vz << std::endl;

            for(uint32_t vz = min_patch_vz; vz <= max_patch_vz; vz++)
            {
                for(uint32_t vx = min_patch_vx; vx <= max_patch_vx; vx++)
                {
                    const vec3 v(m_x + m_patch_size_x * (patch_x + vx / m_vertices[patch_id].tess_level),
                                 m_vertex_data[(tess_level_u32 + 1) * (tess_level_u32 + 1) * patch_id + vz * (tess_level_u32 + 1) + vx].height,
                                 m_z + m_patch_size_z * (patch_z + vz / m_vertices[patch_id].tess_level));

                    if(glm::distance(center, v) <= radius)
                    {
                        m_vertex_data[(tess_level_u32 + 1) * (tess_level_u32 + 1) * patch_id + vz * (tess_level_u32 + 1) + vx].height += dh;

                        m_bounding_ys[patch_id][0] = std::min(m_bounding_ys[patch_id][0], m_vertex_data[(tess_level_u32 + 1) * (tess_level_u32 + 1) * patch_id + vz * (tess_level_u32 + 1) + vx].height);
                        m_bounding_ys[patch_id][1] = std::max(m_bounding_ys[patch_id][1], m_vertex_data[(tess_level_u32 + 1) * (tess_level_u32 + 1) * patch_id + vz * (tess_level_u32 + 1) + vx].height);
                    }
                }
            }
        }
    }

    //TODO: only update the data that's changed
    engine3d.updateTerrainData(m_vertex_data.data(), 0, m_vertex_data.size() * sizeof(VertexData));
}
#endif
