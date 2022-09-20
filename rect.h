#ifndef RECT_H
#define RECT_H

#include "gui_object.h"
#include "color.h"
#include "vertex.h"
#include "engine_3d.h"

class Rect : public GuiObject
{
public:
    Rect(Engine3D& engine3d, float x, float y, float w, float h, ColorRGBA color = ColorRGBA::White);
    Rect(Engine3D& engine3d, float x, float y, float w, float h, TexId tex_id, ColorRGBA color = ColorRGBA::White);

public:
    virtual void update(Engine3D& engine3d, float dt) override;
    virtual void draw(Engine3D& engine3d) override;
    virtual void onResolutionChange(float scale_x, float scale_y, const Font& font) override;

    virtual bool isPointInside(vec2) override;

    void setColor(ColorRGBA);
    void setTexture(TexId);

    virtual void move(vec2) override;

    void setPosAndSize(float x, float y, float width, float height);

    virtual void setScissor(Quad scissor) override;

private:
    void updateTransform();

    float m_x, m_y, m_width, m_height;
    VertexQuad m_vertex;
    VertexBufferAllocation m_vb_alloc;

    std::optional<Quad> m_scissor;
};


#endif // RECT_H
