#include "terrain.h"
#include <game_utils.h>
#include <print>
#include <format>

Terrain::Terrain(Engine3D& engine3d)
{
#if EDITOR_ENABLE
    {
        std::ifstream in(TERRAIN_FILENAME, std::ios::binary);
        if(!in)
        {
            std::println("Failed to open terrain file {}. Creating new terrain...", TERRAIN_FILENAME);
            createNew();
        }
    }
#endif

    loadFromFile(engine3d);
}

void Terrain::calcXYFromSize() noexcept
{
    m_x = -0.5f * m_size;
    m_z = -0.5f * m_size;
}

#if EDITOR_ENABLE
void Terrain::createNew()
{
    m_size = DEFAULT_SIZE;
    calcXYFromSize();
    m_patch_count = DEFAULT_PATCH_COUNT;
    m_patch_size = m_size / static_cast<float>(m_patch_count);

    m_patch_vertices.resize(m_patch_count * m_patch_count);
    m_bounding_ys.resize(m_patch_vertices.size(), vec2(0.0f, 0.0f));
    m_vertex_data.resize((MAX_TESS_LEVEL + 1) * (MAX_TESS_LEVEL + 1) * m_patch_count * m_patch_count);
    m_heightmaps.resize(m_patch_vertices.size());
    for(auto& heightmap : m_heightmaps)
    {
        std::ranges::fill(heightmap, 0.0f);
    }

    for(uint32_t z = 0; z < m_patch_count; z++)
    {
        for(uint32_t x = 0; x < m_patch_count; x++)
        {
            const uint32_t patch_id = z * m_patch_count + x;

            m_patch_vertices[patch_id].pos = vec2(m_x + x * m_patch_size, m_z + z * m_patch_size);
            m_patch_vertices[patch_id].heightmap_id = patch_id;
        }
    }

    saveToFile();
}
#endif

void Terrain::loadFromFile(Engine3D& engine3d)
{
    std::ifstream in(TERRAIN_FILENAME, std::ios::binary);

    //TODO: do better file opening error handlng here
    if(!in)
    {
        error(std::format("Failed to open file {}.", TERRAIN_FILENAME));
    }

    in.read(reinterpret_cast<char*>(&m_size), sizeof(m_size));
    in.read(reinterpret_cast<char*>(&m_patch_count), sizeof(m_patch_count));

    calcXYFromSize();
    m_patch_size = m_size / static_cast<float>(m_patch_count);

    uint64_t bounding_ys_count = 0;
    in.read(reinterpret_cast<char*>(&bounding_ys_count), sizeof(uint64_t));
    m_bounding_ys.resize(bounding_ys_count);
    in.read(reinterpret_cast<char*>(m_bounding_ys.data()), m_bounding_ys.size() * sizeof(vec2));

    uint64_t vertex_data_count = 0;
    in.read(reinterpret_cast<char*>(&vertex_data_count), sizeof(uint64_t));
    m_vertex_data.resize(vertex_data_count);
    in.read(reinterpret_cast<char*>(m_vertex_data.data()), m_vertex_data.size() * sizeof(VertexData));

    m_patch_vertices.resize(m_patch_count * m_patch_count);
    in.read(reinterpret_cast<char*>(m_patch_vertices.data()), m_patch_vertices.size() * sizeof(VertexTerrain));

    m_heightmaps.resize(m_patch_vertices.size());
    in.read(reinterpret_cast<char*>(m_heightmaps.data()), m_heightmaps.size() * sizeof(m_heightmaps[0]));

    m_vb_alloc = engine3d.requestVertexBufferAllocation<VertexTerrain>(m_patch_vertices.size());
    engine3d.updateVertexData(m_vb_alloc.vb, m_vb_alloc.data_offset, sizeof(VertexTerrain) * m_patch_vertices.size(), m_patch_vertices.data());

    engine3d.requestTerrainBufferAllocation(m_vertex_data.size() * sizeof(VertexData));
    engine3d.updateTerrainData(m_vertex_data.data(), 0, m_vertex_data.size() * sizeof(VertexData));

    //TODO:don't do this after we add proper patch streaming
    std::vector<std::pair<float*, uint32_t>> heightmap_reqs(m_heightmaps.size());
    for(size_t i = 0; i < m_heightmaps.size(); i++)
    {
        heightmap_reqs[i] = std::make_pair(m_heightmaps[i].data(), i);
    }
    engine3d.requestTerrainHeightmaps(heightmap_reqs);
}

void Terrain::draw(Engine3D& engine3d)
{
    //TODO: do frustum culling (both camera and light sources views!) and only draw visible patches, also stream heightmaps onto GPU
    //try and compare the following approaches:
    //1. Update vertex buffer with only the patches to draw this frame (the ones that passed frustum culling) and draw the whole VB in a single draw call
    //2. Always have all patch vertex data on the GPU and use an index buffer and update it with indices of vertices that passed frustum culling and are to be drawn this frame and draw in a single draw call
    //3. Always have all patch vertex data on the GPU and issue multiple draw calls to pick the vertices to be drawn this frame that passed frustum culling
    engine3d.updateVertexData(m_vb_alloc.vb, m_vb_alloc.data_offset, sizeof(VertexTerrain) * m_patch_vertices.size(), m_patch_vertices.data());
    engine3d.draw(m_render_mode, m_vb_alloc.vb, m_vb_alloc.vertex_offset, m_patch_vertices.size(), 0, {});
}

float Terrain::patchSize() const
{
    return m_patch_size;
}

float Terrain::collision(const AABB& aabb, const float max_dh) const
{
    const float min_x_coord = std::max<float>((aabb.min().x - m_x) / m_patch_size, 0);
    const float max_x_coord = std::min<float>((aabb.max().x - m_x) / m_patch_size, m_patch_count);
    const float min_z_coord = std::max<float>((aabb.min().z - m_z) / m_patch_size, 0);
    const float max_z_coord = std::min<float>((aabb.max().z - m_z) / m_patch_size, m_patch_count);

    const uint32_t min_patch_x = static_cast<uint32_t>(min_x_coord);
    const uint32_t max_patch_x = std::min<uint32_t>(max_x_coord, m_patch_count - 1);
    const uint32_t min_patch_z = static_cast<uint32_t>(min_z_coord);
    const uint32_t max_patch_z = std::min<uint32_t>(max_z_coord, m_patch_count - 1);

    const float min_vx = min_x_coord - static_cast<float>(min_patch_x);
    const float max_vx = max_x_coord - static_cast<float>(max_patch_x);
    const float min_vz = min_z_coord - static_cast<float>(min_patch_z);
    const float max_vz = max_z_coord - static_cast<float>(max_patch_z);

    float dh = std::numeric_limits<float>::lowest();

    for(uint32_t patch_z = min_patch_z; patch_z <= max_patch_z; patch_z++)
    {
        for(uint32_t patch_x = min_patch_x; patch_x <= max_patch_x; patch_x++)
        {
            const uint32_t patch_id = patch_z * m_patch_count + patch_x;
            //TODO: is bounding_ys even worth using here?
            // const vec2& bounding_ys = m_bounding_ys[patch_id];

//            if((bounding_ys[0] >= aabb.min().y) && (bounding_ys[1] <= aabb.max().y))
            {
                const uint32_t min_patch_vx = (patch_x == static_cast<uint32_t>(min_patch_x)) ? static_cast<uint32_t>(MAX_TESS_LEVEL * min_vx) : 0;
                const uint32_t min_patch_vz = (patch_z == static_cast<uint32_t>(min_patch_z)) ? static_cast<uint32_t>(MAX_TESS_LEVEL * min_vz) : 0;
                const uint32_t max_patch_vx = (patch_x == static_cast<uint32_t>(max_patch_x)) ? std::min<uint32_t>(MAX_TESS_LEVEL, static_cast<uint32_t>(MAX_TESS_LEVEL * max_vx) + 1) : MAX_TESS_LEVEL;
                const uint32_t max_patch_vz = (patch_z == static_cast<uint32_t>(max_patch_z)) ? std::min<uint32_t>(MAX_TESS_LEVEL, static_cast<uint32_t>(MAX_TESS_LEVEL * max_vz) + 1) : MAX_TESS_LEVEL;

                for(uint32_t vz = min_patch_vz; vz <= max_patch_vz; vz++)
                {
                    for(uint32_t vx = min_patch_vx; vx <= max_patch_vx; vx++)
                    {
                        const float h = m_heightmaps[patch_id][vz * (MAX_TESS_LEVEL + 1) + vx];
                        const float dh_ = h - aabb.min().y;
                        if(dh_ > dh)
                        {
                            //early out TODO: check if optimal
                            if(dh_ > max_dh)
                            {
                                return dh_;
                            }
                            dh = dh_;
                        }
                    }
                }
            }
        }
    }

    return dh;

#if 0
    const float min_x_coord = std::max<float>((aabb.min().x - m_x) / m_patch_size, 0);
    const float max_x_coord = std::min<float>((aabb.max().x - m_x) / m_patch_size, m_patch_count);
    const float min_z_coord = std::max<float>((aabb.min().z - m_z) / m_patch_size, 0);
    const float max_z_coord = std::min<float>((aabb.max().z - m_z) / m_patch_size, m_patch_count);

    const uint32_t min_patch_x = min_x_coord;
    const uint32_t max_patch_x = std::min<uint32_t>(max_x_coord, m_patch_count - 1);
    const uint32_t min_patch_z = min_z_coord;
    const uint32_t max_patch_z = std::min<uint32_t>(max_z_coord, m_patch_count - 1);

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
            const uint32_t patch_id = patch_z * m_patch_count + patch_x;
            //TODO: is bounding_ys even worth using here?
            const vec2& bounding_ys = m_bounding_ys[patch_id];

//            if((bounding_ys[0] >= aabb.min().y) && (bounding_ys[1] <= aabb.max().y))
            {
                const uint32_t res_u32 = static_cast<uint32_t>(m_patch_vertices[patch_id].res);

                const uint32_t min_patch_vx = (patch_x == static_cast<uint32_t>(min_patch_x)) ? static_cast<uint32_t>(m_patch_vertices[patch_id].res * min_vx) : 0;
                const uint32_t min_patch_vz = (patch_z == static_cast<uint32_t>(min_patch_z)) ? static_cast<uint32_t>(m_patch_vertices[patch_id].res * min_vz) : 0;
                const uint32_t max_patch_vx = (patch_x == static_cast<uint32_t>(max_patch_x)) ? std::min<uint32_t>(res_u32, static_cast<uint32_t>(m_patch_vertices[patch_id].res * max_vx) + 1) : res_u32;
                const uint32_t max_patch_vz = (patch_z == static_cast<uint32_t>(max_patch_z)) ? std::min<uint32_t>(res_u32, static_cast<uint32_t>(m_patch_vertices[patch_id].res * max_vz) + 1) : res_u32;

                /*test the vertices inside the bounding volume*/
                if((max_patch_vz - min_patch_vz) > 4 && (max_patch_vx - min_patch_vx) > 4)
                {
                    for(uint32_t vz = min_patch_vz + 2; vz <= max_patch_vz - 2; vz++)
                    {
                        for(uint32_t vx = min_patch_vx + 2; vx <= max_patch_vx - 2; vx++)
                        {
                            const float vy = m_vertex_data[m_patch_vertices[patch_id].vertex_offset + vz * (res_u32 + 1) + vx].height;
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
                const float dx = m_patch_size / m_patch_vertices[patch_id].res;
                const float dz = m_patch_size / m_patch_vertices[patch_id].res;

                auto test_triangles = [&](float vx, float vz)
                {
                    const uint32_t base_vid = m_patch_vertices[patch_id].vertex_offset + vz * (res_u32 + 1) + vx;
                    const vec3 v0(m_x + m_patch_size * patch_x + dx * vx, m_vertex_data[base_vid].height, m_z + m_patch_size * patch_z + dz * vz);
                    const vec3 v1(m_x + m_patch_size * patch_x + dx * vx, m_vertex_data[base_vid + res_u32 + 1].height, m_z + m_patch_size * patch_z + dz * (vz + 1));
                    const vec3 v2(m_x + m_patch_size * patch_x + dx * (vx + 1), m_vertex_data[base_vid + res_u32 + 2].height, m_z + m_patch_size * patch_z + dz * (vz + 1));
                    const vec3 v3(m_x + m_patch_size * patch_x + dx * (vx + 1), m_vertex_data[base_vid + 1].height, m_z + m_patch_size * patch_z + dz * vz);

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
#endif
}

#if EDITOR_ENABLE

void Terrain::saveToFile()
{
    //TODO: add error handling
    std::ofstream out(TERRAIN_FILENAME, std::ios::binary);

    out.write(reinterpret_cast<const char*>(&m_size), sizeof(m_size));
    out.write(reinterpret_cast<const char*>(&m_patch_count), sizeof(m_patch_count));

    const uint64_t bounding_ys_count = m_bounding_ys.size();
    out.write(reinterpret_cast<const char*>(&bounding_ys_count), sizeof(uint64_t));
    out.write(reinterpret_cast<const char*>(m_bounding_ys.data()), m_bounding_ys.size() * sizeof(vec2));
    const uint64_t vertex_data_count = m_vertex_data.size();
    out.write(reinterpret_cast<const char*>(&vertex_data_count), sizeof(uint64_t));
    out.write(reinterpret_cast<const char*>(m_vertex_data.data()), m_vertex_data.size() * sizeof(VertexData));
    out.write(reinterpret_cast<const char*>(m_patch_vertices.data()), m_patch_vertices.size() * sizeof(VertexTerrain));
    out.write(reinterpret_cast<const char*>(m_heightmaps.data()), m_heightmaps.size() * sizeof(m_heightmaps[0]));
}

void Terrain::setSize(Engine3D& engine3d, float size)
{
    m_size = size;
    m_x = -0.5f * m_size;
    m_z = -0.5f * m_size;
    m_patch_size = m_size / static_cast<float>(m_patch_count);

    for(uint32_t z = 0; z < m_patch_count; z++)
    {
        for(uint32_t x = 0; x < m_patch_count; x++)
        {
            const uint32_t patch_id = z * m_patch_count + x;
            const vec3 pos = vec3(m_x + x * m_patch_size, 0.0f, m_z + z * m_patch_size);

            m_patch_vertices[patch_id].pos = vec2(pos.x, pos.z);
        }
    }

    engine3d.updateVertexData(m_vb_alloc.vb, m_vb_alloc.data_offset, sizeof(VertexTerrain) * m_patch_vertices.size(), m_patch_vertices.data());
}

bool Terrain::rayIntersection(const Ray& ray, float& d) const
{
#if 0
    d = std::numeric_limits<float>::max();
    bool intersection_found = false;

    for(uint32_t patch_z = 0; patch_z < m_patch_count; patch_z++)
    {
        for(uint32_t patch_x = 0; patch_x < m_patch_count; patch_x++)
        {
            const uint32_t patch_id = patch_z * m_patch_count + patch_x;
            const AABB aabb(vec3(m_x + m_patch_size * patch_x, m_bounding_ys[patch_id][0], m_z + m_patch_size * patch_z), vec3(m_x + m_patch_size * (patch_x + 1.0f), m_bounding_ys[patch_id][1], m_z + m_patch_size * (patch_z + 1.0f)));

            if(intersect(ray, aabb))
            {
                const uint32_t res_u32 = static_cast<uint32_t>(m_patch_vertices[patch_id].res);
                const float dx = m_patch_size / m_patch_vertices[patch_id].res;
                const float dz = m_patch_size / m_patch_vertices[patch_id].res;

                for(uint32_t x = 0; x < static_cast<uint32_t>(m_patch_vertices[patch_id].res); x++)
                {
                    for(uint32_t z = 0; z < static_cast<uint32_t>(m_patch_vertices[patch_id].res); z++)
                    {
                        const vec3 v0 = vec3(m_x + m_patch_size * patch_x + dx * x,
                                             m_vertex_data[(res_u32 + 1) * (res_u32 + 1) * patch_id + z * (res_u32 + 1) + x].height,
                                             m_z + m_patch_size * patch_z + dz * z);
                        const vec3 v1 = vec3(m_x + m_patch_size * patch_x + dx * (x + 1),
                                             m_vertex_data[(res_u32 + 1) * (res_u32 + 1) * patch_id + z * (res_u32 + 1) + (x + 1)].height,
                                             m_z + m_patch_size * patch_z + dz * z);
                        const vec3 v2 = vec3(m_x + m_patch_size * patch_x + dx * x,
                                             m_vertex_data[(res_u32 + 1) * (res_u32 + 1) * patch_id + (z + 1) * (res_u32 + 1) + x].height,
                                             m_z + m_patch_size * patch_z + dz * (z + 1));
                        const vec3 v3 = vec3(m_x + m_patch_size * patch_x + dx * (x + 1),
                                             m_vertex_data[(res_u32 + 1) * (res_u32 + 1) * patch_id + (z + 1) * (res_u32 + 1) + (x + 1)].height,
                                             m_z + m_patch_size * patch_z + dz * (z + 1));

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
#endif
}

void Terrain::toolEdit(Engine3D& engine3d, const vec3& center, float radius, float dh)
{
#if 0
    const float min_x_coord = std::max<float>((center.x - radius - m_x) / m_patch_size, 0);
    const float max_x_coord = std::min<float>((center.x + radius - m_x) / m_patch_size, m_patch_count);
    const float min_z_coord = std::max<float>((center.z - radius - m_z) / m_patch_size, 0);
    const float max_z_coord = std::min<float>((center.z + radius - m_z) / m_patch_size, m_patch_count);

    const uint32_t min_patch_x = min_x_coord;
    const uint32_t max_patch_x = std::min<uint32_t>(max_x_coord, m_patch_count - 1);
    const uint32_t min_patch_z = min_z_coord;
    const uint32_t max_patch_z = std::min<uint32_t>(max_z_coord, m_patch_count - 1);;

    const float min_vx = min_x_coord - static_cast<float>(min_patch_x);
    const float max_vx = max_x_coord - static_cast<float>(max_patch_x);
    const float min_vz = min_z_coord - static_cast<float>(min_patch_z);
    const float max_vz = max_z_coord - static_cast<float>(max_patch_z);

    for(uint32_t patch_z = min_patch_z; patch_z <= max_patch_z; patch_z++)
    {
        for(uint32_t patch_x = min_patch_x; patch_x <= max_patch_x; patch_x++)
        {
            const uint32_t patch_id = patch_z * m_patch_count + patch_x;

            const uint32_t res_u32 = static_cast<uint32_t>(m_patch_vertices[patch_id].res);

            const uint32_t min_patch_vx = (patch_x == min_patch_x) ? static_cast<uint32_t>(m_patch_vertices[patch_id].res * min_vx) : 0;
            const uint32_t min_patch_vz = (patch_z == min_patch_z) ? static_cast<uint32_t>(m_patch_vertices[patch_id].res * min_vz) : 0;
            const uint32_t max_patch_vx = (patch_x == max_patch_x) ? std::min<uint32_t>(res_u32, static_cast<uint32_t>(m_patch_vertices[patch_id].res * max_vx) + 1) : res_u32;
            const uint32_t max_patch_vz = (patch_z == max_patch_z) ? std::min<uint32_t>(res_u32, static_cast<uint32_t>(m_patch_vertices[patch_id].res * max_vz) + 1) : res_u32;

            for(uint32_t vz = min_patch_vz; vz <= max_patch_vz; vz++)
            {
                for(uint32_t vx = min_patch_vx; vx <= max_patch_vx; vx++)
                {
                    const vec3 v(m_x + m_patch_size * (patch_x + vx / m_patch_vertices[patch_id].res),
                                 m_vertex_data[(res_u32 + 1) * (res_u32 + 1) * patch_id + vz * (res_u32 + 1) + vx].height,
                                 m_z + m_patch_size * (patch_z + vz / m_patch_vertices[patch_id].res));

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
#endif
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
