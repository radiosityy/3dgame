#include "mesh.h"
#include <fstream>

Mesh::Mesh(Engine3D& engine3d, std::ifstream& model_file, uint32_t bone_offset)
{
    uint8_t texture_filename_length = 0;
    model_file.read(reinterpret_cast<char*>(&texture_filename_length), sizeof(uint8_t));

    if(texture_filename_length > 0)
    {
        std::string texture_filename;
        texture_filename.resize(texture_filename_length);
        model_file.read(reinterpret_cast<char*>(texture_filename.data()), texture_filename_length);
        m_tex_id = engine3d.loadTexture(texture_filename);
    }

    uint8_t normal_map_filename_length = 0;
    model_file.read(reinterpret_cast<char*>(&normal_map_filename_length), sizeof(uint8_t));

    if(normal_map_filename_length > 0)
    {
        std::string normal_map_filename;
        normal_map_filename.resize(normal_map_filename_length);
        model_file.read(reinterpret_cast<char*>(normal_map_filename.data()), normal_map_filename_length);
        m_normal_map_id = engine3d.loadNormalMap(normal_map_filename);
    }

    uint64_t vertex_count = 0;
    model_file.read(reinterpret_cast<char*>(&vertex_count), sizeof(uint64_t));

    m_vertex_data.resize(vertex_count);

    //TODO: read the vertex data in a single read() rather than in a loop
    for(uint64_t i = 0; i < vertex_count; i++)
    {
        model_file.read(reinterpret_cast<char*>(&m_vertex_data[i].pos), sizeof(vec3));
        model_file.read(reinterpret_cast<char*>(&m_vertex_data[i].normal), sizeof(vec3));
        model_file.read(reinterpret_cast<char*>(&m_vertex_data[i].tangent), sizeof(vec3));
        model_file.read(reinterpret_cast<char*>(&m_vertex_data[i].tex_coords), sizeof(vec2));
        model_file.read(reinterpret_cast<char*>(&m_vertex_data[i].bone_id), sizeof(uint8_t));
        m_vertex_data[i].bone_id += bone_offset;
    }

    m_vb_alloc = engine3d.requestVertexBufferAllocation<VertexDefault>(vertex_count);
    engine3d.updateVertexData(m_vb_alloc.vb, m_vb_alloc.data_offset, sizeof(VertexDefault) * vertex_count, m_vertex_data.data());
}

uint32_t Mesh::textureId() const
{
    return m_tex_id;
}

uint32_t Mesh::normalMapId() const
{
    return m_normal_map_id;
}

VertexBuffer* Mesh::vertexBuffer() const
{
    return m_vb_alloc.vb;
}

uint32_t Mesh::vertexBufferOffset() const
{
    return m_vb_alloc.vertex_offset;
}

uint32_t Mesh::vertexCount() const
{
    return m_vertex_data.size();
}

const std::vector<VertexDefault>& Mesh::vertexData() const
{
    return m_vertex_data;
}
