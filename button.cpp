#include "button.h"

Button::Button(Engine3D& engine3d, float x, float y, float w, float h, const Font& font, const std::string& text, std::move_only_function<void()>&& mouse_pressed_callback, HorizontalAlignment hor_align, VerticalAlignment ver_align)
    : m_x(x)
    , m_y(y)
    , m_width(w)
    , m_height(h)
    , m_label(engine3d, m_x, m_y, m_width, m_height, font, text, false, hor_align, ver_align)
    , m_mouse_pressed_callback(std::move(mouse_pressed_callback))
{
    m_label.setScissor({m_x, m_y, m_width, m_height});
    m_label.setBackgroundColor(m_color);
}

void Button::setUpdateCallback(std::move_only_function<void (Button&)>&& callback)
{
    m_update_callback = std::move(callback);
}

void Button::update(Engine3D& engine3d, float dt)
{
    if(m_update_callback)
    {
        m_update_callback(*this);
    }

    m_label.update(engine3d, dt);
}

void Button::draw(Engine3D& engine3d)
{
    m_label.draw(engine3d);
}

void Button::onInputEvent(const Event& event, const InputState&)
{
    if((EventType::MousePressed == event.event) && (LMB == event.mouse))
    {
        m_label.setBackgroundColor(m_pressed_color);
    }
    else if((EventType::MouseReleased == event.event) && (LMB == event.mouse))
    {
        m_label.setBackgroundColor(m_highlighted_color);
        m_mouse_pressed_callback();
    }
    else if(EventType::KeyPressed == event.event)
    {
        if(event.key == VKeyEnter)
        {
            m_mouse_pressed_callback();
        }
    }
}

void Button::cursorEnter()
{
    m_label.setBackgroundColor(m_highlighted_color);
}

void Button::cursorExit()
{
    m_label.setBackgroundColor(m_color);
}

void Button::gotFocus()
{
    m_label.setBackgroundColor(m_highlighted_color);
}

void Button::lostFocus()
{
    m_label.setBackgroundColor(m_color);
}

bool Button::isPointInside(vec2 p)
{
    return m_label.isPointInside(p);
}

void Button::setColor(const ColorRGBA& color)
{
    m_color = color;
    m_label.setBackgroundColor(color);
}

void Button::setHighlightColor(const ColorRGBA& color)
{
    m_highlighted_color = color;
}

void Button::setPressedColor(const ColorRGBA& color)
{
    m_pressed_color = color;
}

void Button::setTexture(TexId tex)
{
    m_label.setBackgroundTex(tex);
}

void Button::setScissor(Quad scissor)
{
    m_scissor =  scissor;
    m_label.setScissor(quadOverlap({m_x, m_y, m_width, m_height}, *m_scissor));
}
