#include "checkbox.h"

Checkbox::Checkbox(Renderer& renderer, float x, float y, float w, float h, bool init_checked)
    : Rect(renderer, x, y, w, h)
    , m_checked(init_checked)
    , m_unchecked_tex(TexId(renderer.loadTexture("checkbox_unchecked.png"), 0))
    , m_checked_tex(TexId(renderer.loadTexture("checkbox_checked.png"), 0))
{
    if(m_checked)
    {
        setTexture(m_checked_tex);
    }
    else
    {
        setTexture(m_unchecked_tex);
    }
}

void Checkbox::onMouseReleased(MouseButton mb, const InputState& input_state, bool released_inside)
{
    if(LMB == mb)
    {
        if(released_inside)
        {
            toggle();
        }
    }
}

bool Checkbox::state() const
{
    return m_checked;
}

void Checkbox::setChecked(bool checked)
{
    m_checked = checked;

    if(m_checked)
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
    setChecked(!m_checked);
}

void Checkbox::setOnCheckCallback(std::move_only_function<void()>&& callback)
{
    m_on_check_callback = std::move(callback);
}

void Checkbox::setOnUncheckCallback(std::move_only_function<void()>&& callback)
{
    m_on_uncheck_callback = std::move(callback);
}
