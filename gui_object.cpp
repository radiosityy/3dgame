#include "gui_object.h"

#include <tuple>
#include <fstream>

void GuiObject::onInputEvent(const Event&, const InputState&)
{
}

void GuiObject::gotFocus()
{
}

void GuiObject::lostFocus()
{
}

void GuiObject::cursorEnter()
{
}

void GuiObject::cursorExit()
{
}

void GuiObject::updateAndDraw(Engine3D& engine3d, float dt)
{
    update(engine3d, dt);
    draw(engine3d);
}

bool GuiObject::isVisible() const noexcept
{
    return m_visible;
}

void GuiObject::setVisible(bool visible) noexcept
{
    m_visible = visible;
}

bool GuiObject::isFocusable() const noexcept
{
    return m_focusable;
}

void GuiObject::setFocusable(bool focusable) noexcept
{
    m_focusable = focusable;
}

void GuiObject::move(vec2)
{
}

void GuiObject::scale(vec2)
{
}

void GuiParent::setKeyboardFocus(GuiObject* object)
{
    if(object)
    {
        if(m_keyboard_focus)
        {
            if(m_keyboard_focus != object)
            {
                m_keyboard_focus->lostFocus();
                m_keyboard_focus = object;
                m_keyboard_focus->gotFocus();
            }
        }
        else
        {
            m_keyboard_focus = object;
            m_keyboard_focus->gotFocus();
        }
    }
    else if(m_keyboard_focus)
    {
        m_keyboard_focus->lostFocus();
        m_keyboard_focus = nullptr;
    }
}

void GuiParent::setMouseFocus(GuiObject* object)
{
    if(object)
    {
        if(m_mouse_focus)
        {
            if(m_mouse_focus != object)
            {
                m_mouse_focus->cursorExit();
                m_mouse_focus = object;
                m_mouse_focus->cursorEnter();
            }
        }
        else
        {
            m_mouse_focus = object;
            m_mouse_focus->cursorEnter();
        }
    }
    else if(m_mouse_focus)
    {
        m_mouse_focus->cursorExit();
        m_mouse_focus = nullptr;
    }
}

void GuiParent::resetKeyboardFocus()
{
    setKeyboardFocus(nullptr);
}

void GuiParent::resetMouseFocus()
{
    setMouseFocus(nullptr);
}

void GuiParent::determineKeyboardFocus(GuiObject* object, const Event& event, const InputState& input_state)
{
    if((EventType::MousePressed == event.event) && (LMB == event.mouse))
    {
        if(object->isVisible() && object->isFocusable() && object->isPointInside(input_state.cursor_pos))
        {
            setKeyboardFocus(object);
        }
        else
        {
            setKeyboardFocus(nullptr);
        }
    }
}

void GuiParent::determineMouseFocus(GuiObject* object, const Event& event, const InputState& input_state)
{
    if(EventType::MouseMoved == event.event)
    {
        if(object->isVisible() && object->isFocusable() && object->isPointInside(input_state.cursor_pos))
        {
            setMouseFocus(object);
        }
        else
        {
            setMouseFocus(nullptr);
        }
    }
}

bool GuiParent::forwardEventToFocused(const Event& event, const InputState& input_state)
{
    if(event.keyEvent() && m_keyboard_focus)
    {
        m_keyboard_focus->onInputEvent(event, input_state);
        return true;
    }
    else if(event.mouseEvent() && m_mouse_focus)
    {
        m_mouse_focus->onInputEvent(event, input_state);
        return true;
    }

    return false;
}

void GuiParentObject::lostFocus()
{
    if(m_keyboard_focus)
    {
        m_keyboard_focus->lostFocus();
        m_keyboard_focus = nullptr;
    }
}

void GuiParentObject::cursorExit()
{
    if(m_mouse_focus)
    {
        m_mouse_focus->cursorExit();
        m_mouse_focus = nullptr;
    }
}
