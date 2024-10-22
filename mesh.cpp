#include "mesh.h"
#include <fstream>

Mesh::Mesh(Renderer& renderer, std::ifstream& model_file)
{
    uint8_t texture_filename_length = 0;
    model_file.read(reinterpret_cast<char*>(&texture_filename_length), sizeof(uint8_t));

    if(texture_filename_length > 0)
    {
        std::string texture_filename;
        texture_filename.resize(texture_filename_length);
        model_file.read(reinterpret_cast<char*>(texture_filename.data()), texture_filename_length);
        m_tex_id = renderer.loadTexture(texture_filename);
    }

    uint8_t normal_map_filename_length = 0;
    model_file.read(reinterpret_cast<char*>(&normal_map_filename_length), sizeof(uint8_t));

    if(normal_map_filename_length > 0)
    {
        std::string normal_map_filename;
        normal_map_filename.resize(normal_map_filename_length);
        model_file.read(reinterpret_cast<char*>(normal_map_filename.data()), normal_map_filename_length);
        m_normal_map_id = renderer.loadNormalMap(normal_map_filename);
    }

    uint64_t vertex_count = 0;
    model_file.read(reinterpret_cast<char*>(&vertex_count), sizeof(uint64_t));

    m_vertex_data.resize(vertex_count);
    model_file.read(reinterpret_cast<char*>(m_vertex_data.data()), vertex_count * sizeof(VertexDefault));

    m_vb_alloc = renderer.reqVBAlloc<VertexDefault>(vertex_count);
    renderer.updateVertexData(m_vb_alloc.vb, m_vb_alloc.data_offset, sizeof(VertexDefault) * vertex_count, m_vertex_data.data());
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
