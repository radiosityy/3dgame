#include "rect.h"

Rect::Rect(Renderer& renderer, float x, float y, float w, float h, ColorRGBA color, Quad scissor)
    : m_x(x)
    , m_y(y)
    , m_width(w)
    , m_height(h)
    , m_scissor(scissor)
{
    m_vb_alloc = renderer.requestVertexBufferAllocation<VertexUi>(1);

    m_vertex.use_texture = 0;
    setFocusable(false);
    setColor(color);
    updateVertex();
}

Rect::Rect(Renderer& renderer, float x, float y, float w, float h, TexId tex_id, ColorRGBA color, Quad scissor)
    : m_x(x)
    , m_y(y)
    , m_width(w)
    , m_height(h)
    , m_scissor(scissor)
{
    m_vb_alloc = renderer.requestVertexBufferAllocation<VertexUi>(1);

    setFocusable(false);
    setColor(color);
    setTexture(tex_id);
    updateVertex();
}

void Rect::update(Renderer& renderer)
{
    //TODO: only update vertex data when it changes
    renderer.updateVertexData(m_vb_alloc.vb, m_vb_alloc.data_offset, sizeof(VertexUi), &m_vertex);
}

void Rect::draw(Renderer& renderer)
{
    renderer.drawUi(RenderModeUi::Ui, m_vb_alloc.vb, m_vb_alloc.vertex_offset, 1, m_scissor);
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

    updateVertex();
}

void Rect::setPosAndSize(float x, float y, float width, float height)
{
    m_x = x;
    m_y = y;
    m_width = width;
    m_height = height;

    updateVertex();
}

void Rect::setScissor(Quad scissor)
{
    m_scissor = scissor;
}

void Rect::updateVertex()
{
    m_vertex.size = vec2(m_width, m_height);
    m_vertex.top_left_pos = vec2(m_x, m_y);
}
