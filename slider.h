#ifndef SLIDER_H
#define SLIDER_H

#include "gui_object.h"
#include "rect.h"
#include "engine_3d.h"

template<class T = float, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
class Slider : public GuiObject
{
public:
    Slider(Engine3D& engine3d, float x, float y, float w, float h, T min_value, T max_value, T init_value, std::move_only_function<void(T)>&& value_changed_callback = {})
        : m_handle_width(h)
        , m_bar_width(w - m_handle_width)
        , m_bar_x(x + 0.5f * m_handle_width)
        , m_x(x)
        , m_y(y)
        , m_width(w)
        , m_height(h)
        , m_value_changed_callback(std::move(value_changed_callback))
        , m_value(init_value)
        , m_min_value(min_value)
        , m_value_range(max_value - min_value)
        , m_bar(engine3d, m_bar_x, m_y + 0.5f * (1.0f - m_bar_height) * m_height, m_width - m_handle_width, m_bar_height * m_height, ColorRGBA(0.1f, 0.1f, 0.1f, 1.0f))
        , m_handle(engine3d, 0.0f, 0.0f, 0.0f, 0.0f, TexId(engine3d.loadTexture("circle.png"), 0))
    {
        updateHandle();
    }

    virtual bool isPointInside(vec2 p) override
    {
        const auto d = p - vec2(m_x, m_y);
        return (d.x > 0) && (d.x <= m_width) && (d.y > 0) && (d.y <= m_height);
    }

    virtual void onInputEvent(const Event& event, const InputState& input_state) override
    {
        if(((EventType::MousePressed == event.event) && (LMB == event.mouse)) ||
           ((EventType::MouseDragged == event.event) && (input_state.mouse & LMB)))
        {
            reactToCursor(input_state.cursor_pos);
        }
    }

    void setUpdateCallback(std::move_only_function<void(Slider<T>&)>&& callback)
    {
        m_update_callback = std::move(callback);
    }

    virtual void update(Engine3D& engine3d, float dt) override
    {
        if(m_update_callback)
        {
            m_update_callback(*this);
        }

        m_bar.update(engine3d, dt);
        m_handle.update(engine3d, dt);
    }

    virtual void draw(Engine3D& engine3d) override
    {
        m_bar.draw(engine3d);
        m_handle.draw(engine3d);
    }

    void setValue(T value) noexcept
    {
        m_value = value;

        updateHandle();
    }

    T value() const noexcept
    {
        return m_value;
    }

    virtual void setScissor(Quad scissor) override
    {
        const Quad q = {m_x, m_y, m_width, m_height};
        m_scissor = quadOverlap(q, scissor);

        m_bar.setScissor(*m_scissor);
        m_handle.setScissor(*m_scissor);
    }

private:
    void reactToCursor(vec2 cursor_pos)
    {
        const float offset = std::clamp((cursor_pos.x - m_bar_x) / m_bar_width, 0.0f, 1.0f);
        m_value = m_min_value + offset * m_value_range;
        updateHandle();

        if(m_value_changed_callback)
        {
            m_value_changed_callback(m_value);
        }
    }

    void updateHandle()
    {
        const float offset = std::clamp((m_value - m_min_value) / m_value_range, 0.0f, 1.0f);
        const float x = m_x + offset * m_bar_width;

        m_handle.setPosAndSize(x, m_y, m_handle_width, m_height);
    }

    const float m_handle_width;
    const float m_bar_width;
    const float m_bar_x;
    float m_x, m_y, m_width, m_height;

    static inline const float m_bar_height = 0.25f;

    std::move_only_function<void(Slider<T>&)> m_update_callback;
    std::move_only_function<void(T)> m_value_changed_callback;

    T m_value;
    const T m_min_value;
    const T m_value_range;

    Rect m_bar;
    Rect m_handle;

    std::optional<Quad> m_scissor;
};

#endif // SLIDER_H
