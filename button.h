#ifndef BUTTON_H
#define BUTTON_H

#include "label.h"

class Button : public GuiObject
{
public:
    Button(Renderer& renderer, float x, float y, float w, float h, const Font& font, const std::string& text, std::move_only_function<void()>&& pressed_callback, HorizontalAlignment = HorizontalAlignment::Center, VerticalAlignment = VerticalAlignment::Center);

public:
    virtual void update(Renderer& renderer) override;
    virtual void draw(Renderer& renderer) override;

    virtual void onMousePressed(MouseButton, const InputState&) override;
    virtual void onMouseReleased(MouseButton, const InputState&, bool) override;
    virtual void onKeyPressed(Key, const InputState&) override;
    virtual void onCursorEnter() override;
    virtual void onCursorExit() override;
    virtual void onGotFocus() override;
    virtual void onLostFocus() override;

    virtual bool isPointInside(vec2) override;

    void setColor(const ColorRGBA&);
    void setHighlightColor(const ColorRGBA&);
    void setPressedColor(const ColorRGBA&);
    void setTexture(TexId);

    virtual void setScissor(Quad scissor) override;

protected:
    float m_x, m_y, m_width, m_height;

    ColorRGBA m_color = {0.3f, 0.3f, 0.4f, 1.0f};
    ColorRGBA m_highlighted_color = {0.6f, 0.6f, 0.7f, 1.0f};
    ColorRGBA m_pressed_color = {0.2f, 0.2f, 0.3f, 1.0f};

    Label m_label;

    std::move_only_function<void()> m_pressed_callback;

    std::optional<Quad> m_scissor;
};


#endif // BUTTON_H
