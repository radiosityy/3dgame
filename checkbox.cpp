#include "checkbox.h"

Checkbox::Checkbox(Engine3D& engine3d, float x, float y, float w, float h, bool init_state)
    : Rect(engine3d, x, y, w, h)
    , m_state(init_state)
    , m_unchecked_tex(TexId(engine3d.loadTexture("checkbox_unchecked.png"), 0))
    , m_checked_tex(TexId(engine3d.loadTexture("checkbox_checked.png"), 0))
{
    if(m_state)
    {
        setTexture(m_checked_tex);
    }
    else
    {
        setTexture(m_unchecked_tex);
    }
}

void Checkbox::onInputEvent(const Event& event, const InputState& input_state)
{
    if(event.event == EventType::MouseReleased && event.mouse == LMB)
    {
        toggle();
    }
}

bool Checkbox::state() const
{
    return m_state;
}

void Checkbox::setState(bool state)
{
    m_state = state;

    if(m_state)
    {
        setTexture(m_checked_tex);

        if(m_on_check_callback)
        {
            m_on_check_callback();
        }
    }
    else
    {
        setTexture(m_unchecked_tex);

        if(m_on_uncheck_callback)
        {
            m_on_uncheck_callback();
        }
    }
}

void Checkbox::toggle()
{
    setState(!m_state);
}

void Checkbox::setOnCheckCallback(std::move_only_function<void()>&& callback)
{
    m_on_check_callback = std::move(callback);
}

void Checkbox::setOnUncheckCallback(std::move_only_function<void()>&& callback)
{
    m_on_uncheck_callback = std::move(callback);
}
