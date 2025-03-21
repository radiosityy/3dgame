#ifndef RECT_H
#define RECT_H

#include "gui_object.h"
#include "color.h"
#include "vertex.h"
#include "renderer.h"

class Rect : public GuiObject
{
public:
    Rect(Renderer& renderer, float x, float y, float w, float h, ColorRGBA color = ColorRGBA::White, Quad scissor = Quad::defaultScissor());
    Rect(Renderer& renderer, float x, float y, float w, float h, TexId tex_id, ColorRGBA color = ColorRGBA::White, Quad scissor = Quad::defaultScissor());

public:
    virtual void update(Renderer& renderer) override;
    virtual void draw(Renderer& renderer) override;

    virtual bool isPointInside(vec2) override;

    void setColor(ColorRGBA);
    void setTexture(TexId);

    virtual void move(vec2) override;

    void setPosAndSize(float x, float y, float width, float height);

    virtual void setScissor(Quad scissor) override;

private:
    void updateVertexSizeAndPos();

    //TODO: x,y,width,height members are duplicates of data we already have in m_vertex
    float m_x, m_y, m_width, m_height;
    VertexUi m_vertex;
    VertexBufferAllocation m_vb_alloc;
    bool m_vertex_data_updated = false;

    Quad m_scissor;
};


#endif // RECT_H
