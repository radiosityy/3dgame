#ifndef RECT_H
#define RECT_H

#include "gui_object.h"
#include "color.h"
#include "vertex.h"
#include "engine_3d.h"

class Rect : public GuiObject
{
public:
    Rect(Engine3D& engine3d, float x, float y, float w, float h, ColorRGBA color = ColorRGBA::White, Quad scissor = Quad::defaultScissor());
    Rect(Engine3D& engine3d, float x, float y, float w, float h, TexId tex_id, ColorRGBA color = ColorRGBA::White, Quad scissor = Quad::defaultScissor());

public:
    virtual void update(Engine3D& engine3d, float dt) override;
    virtual void draw(Engine3D& engine3d) override;

    virtual bool isPointInside(vec2) override;

    void setColor(ColorRGBA);
    void setTexture(TexId);

    virtual void move(vec2) override;

    void setPosAndSize(float x, float y, float width, float height);

    virtual void setScissor(Quad scissor) override;

private:
    void updateVertex();

    float m_x, m_y, m_width, m_height;
    VertexUi m_vertex;
    VertexBufferAllocation m_vb_alloc;

    Quad m_scissor;
};


#endif // RECT_H
