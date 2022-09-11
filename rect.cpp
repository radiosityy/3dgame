#include "rect.h"

Rect::Rect(Engine3D& engine3d, float x, float y, float w, float h, ColorRGBA color)
    : m_x(x)
    , m_y(y)
    , m_width(w)
    , m_height(h)
{
    m_vb_alloc = engine3d.requestVertexBufferAllocation<VertexQuad>(1);

    m_vertex.use_texture = 0;
    setFocusable(false);
    setColor(color);
    updateTransform();
}

Rect::Rect(Engine3D& engine3d, float x, float y, float w, float h, TexId tex_id, ColorRGBA color)
    : m_x(x)
    , m_y(y)
    , m_width(w)
    , m_height(h)
{
    m_vb_alloc = engine3d.requestVertexBufferAllocation<VertexQuad>(1);

    setFocusable(false);
    setColor(color);
    setTexture(tex_id);
    updateTransform();
}

void Rect::update(Engine3D& engine3d, float dt)
{
    //TODO: only update vertex data when it changes
    engine3d.updateVertexData(m_vb_alloc.vb, m_vb_alloc.data_offset, sizeof(VertexQuad), &m_vertex);
}

void Rect::draw(Engine3D& engine3d)
{
    engine3d.draw(RenderMode::Ui, m_vb_alloc.vb, m_vb_alloc.vertex_offset, 1, 0, {}, m_scissor);
}

void Rect::onResolutionChange(float scale_x, float scale_y, const Font&)
{
    m_x *= scale_x;
    m_width *= scale_x;
    m_y *= scale_y;
    m_height *= scale_y;

    updateTransform();

    if(m_scissor)
    {
        m_scissor->x *= scale_x;
        m_scissor->width *= scale_x;
        m_scissor->y *= scale_y;
        m_scissor->height *= scale_y;
    }
}

bool Rect::isPointInside(vec2 p)
{
    const auto d = p - vec2(m_x, m_y);
    return (d.x > 0) && (d.x <= m_width) && (d.y > 0) && (d.y <= m_height);
}

void Rect::setColor(ColorRGBA color)
{
    m_vertex.color = color;
}

void Rect::setTexture(TexId tex)
{
    m_vertex.tex_id = tex.tex_id;
    m_vertex.layer_id = tex.layer_id;
    m_vertex.use_texture = 1;
}

void Rect::move(vec2 xy)
{
    m_x += xy.x;
    m_y += xy.y;

    updateTransform();
}

void Rect::setPosAndSize(float x, float y, float width, float height)
{
    m_x = x;
    m_y = y;
    m_width = width;
    m_height = height;

    updateTransform();
}

void Rect::setScissor(Quad scissor)
{
    m_scissor = scissor;
}

void Rect::updateTransform()
{
    m_vertex.T = glm::translate(vec3(m_x, m_y, 0.0f)) * glm::scale(vec3(m_width, m_height, 1.0f));
}
