#ifndef MESH_H
#define MESH_H

#include <memory>
#include <string_view>
#include "vertex.h"
#include "collision.h"
#include "shaders/shader_constants.h"
#include "renderer.h"

class Mesh
{
public:
    Mesh(Renderer& renderer, std::ifstream& model_file);

    uint32_t textureId() const;
    uint32_t normalMapId() const;
    VertexBuffer* vertexBuffer() const;
    uint32_t vertexBufferOffset() const;
    uint32_t vertexCount() const;
    const std::vector<VertexDefault>& vertexData() const;

#if EDITOR_ENABLE
    bool rayIntersetion(const Ray& rayL, float min_d, float& d) const;
#endif

private:
    std::vector<VertexDefault> m_vertex_data;
    VertexBufferAllocation m_vb_alloc;

    uint32_t m_tex_id = 0;
    uint32_t m_normal_map_id = NORMAL_MAP_ID_NONE;
};

#endif //MESH_H
