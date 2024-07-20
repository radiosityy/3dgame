#include "gui_object.h"

#include <tuple>
#include <fstream>

void InputHandler::onKeyPressed(Key, const InputState&) {}
void InputHandler::onKeyReleased(Key, const InputState&) {}
void InputHandler::onMousePressed(MouseButton, const InputState&) {}
void InputHandler::onMouseReleased(MouseButton, const InputState&, bool) {}
void InputHandler::onMouseMoved(vec2, const InputState&) {}
void InputHandler::onMouseScrolledUp(const InputState&) {}
void InputHandler::onMouseScrolledDown(const InputState&) {}

void GuiObject::onGotFocus() {}
void GuiObject::onLostFocus() {}
void GuiObject::onCursorEnter() {}
void GuiObject::onCursorExit() {}
void GuiObject::update(Engine3D&) {}

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

void GuiParent::onKeyPressed(Key key, const InputState& input_state)
{
    if(!onKeyPressedIntercept(key, input_state))
    {
        if(m_keyboard_focus)
        {
            m_keyboard_focus->onKeyPressed(key, input_state);
        }
        else
        {
            onKeyPressedImpl(key, input_state);
        }
    }
}

void GuiParent::onKeyReleased(Key key, const InputState& input_state)
{
    if(!onKeyReleasedIntercept(key, input_state))
    {
        if(m_keyboard_focus)
        {
            m_keyboard_focus->onKeyReleased(key, input_state);
        }
        else
        {
            onKeyReleasedImpl(key, input_state);
        }
    }
}

void GuiParent::onMousePressed(MouseButton mb, const InputState& input_state)
{
    if(!onMousePressedIntercept(mb, input_state))
    {
        if(LMB == mb)
        {
            updateKeyboardFocus(input_state);
        }

        if(m_mouse_focus)
        {
            m_mouse_focus->onMousePressed(mb, input_state);
        }
        else
        {
            onMousePressedImpl(mb, input_state);
        }
    }
}

void GuiParent::onMouseReleased(MouseButton mb, const InputState& input_state, bool released_inside)
{
    if(!onMouseReleasedIntercept(mb, input_state, released_inside))
    {
        if(m_mouse_focus)
        {
            if(!released_inside)
            {
                m_mouse_focus->onMouseReleased(mb, input_state, false);
                resetMouseFocus();
            }
            else
            {
                auto curr_mouse_focus = determineMouseFocus(input_state);

                if(m_mouse_focus == curr_mouse_focus)
                {
                    m_mouse_focus->onMouseReleased(mb, input_state, true);
                }
                else
                {
                    m_mouse_focus->onMouseReleased(mb, input_state, false);
                    setMouseFocus(curr_mouse_focus);
                }
            }
        }
        else
        {
            onMouseReleasedImpl(mb, input_state, released_inside);
        }
    }

    if(released_inside)
    {
        updateMouseFocus(input_state);
    }
}

void GuiParent::onMouseMoved(vec2 cursor_delta, const InputState& input_state)
{
    if(!onMouseMovedIntercept(cursor_delta, input_state))
    {
        if(!input_state.mouse)
        {
            updateMouseFocus(input_state);
        }

        if(m_mouse_focus)
        {
            m_mouse_focus->onMouseMoved(cursor_delta, input_state);
        }
        else
        {
            onMouseMovedImpl(cursor_delta, input_state);
        }
    }
}

void GuiParent::onMouseScrolledUp(const InputState& input_state)
{
    if(!onMouseScrolledUpIntercept(input_state))
    {
        if(m_mouse_focus)
        {
            m_mouse_focus->onMouseScrolledUp(input_state);
        }
        else
        {
            onMouseScrolledUpImpl(input_state);
        }
    }
}

void GuiParent::onMouseScrolledDown(const InputState& input_state)
{
    if(!onMouseScrolledDownIntercept(input_state))
    {
        if(m_mouse_focus)
        {
            m_mouse_focus->onMouseScrolledDown(input_state);
        }
        else
        {
            onMouseScrolledDownImpl(input_state);
        }
    }
}

bool GuiParent::onKeyPressedIntercept(Key, const InputState&) {return false;}
bool GuiParent::onKeyReleasedIntercept(Key, const InputState&) {return false;}
bool GuiParent::onMousePressedIntercept(MouseButton, const InputState&) {return false;}
bool GuiParent::onMouseReleasedIntercept(MouseButton, const InputState&, bool) {return false;}
bool GuiParent::onMouseMovedIntercept(vec2, const InputState&) {return false;}
bool GuiParent::onMouseScrolledUpIntercept(const InputState&) {return false;}
bool GuiParent::onMouseScrolledDownIntercept(const InputState&) {return false;}

void GuiParent::onKeyPressedImpl(Key, const InputState&) {}
void GuiParent::onKeyReleasedImpl(Key, const InputState&) {}
void GuiParent::onMousePressedImpl(MouseButton, const InputState&) {}
void GuiParent::onMouseReleasedImpl(MouseButton, const InputState&, bool) {}
void GuiParent::onMouseMovedImpl(vec2, const InputState&) {}
void GuiParent::onMouseScrolledUpImpl(const InputState&) {}
void GuiParent::onMouseScrolledDownImpl(const InputState&) {}

void GuiParent::removeChild(GuiObject* child)
{
    auto itr = std::ranges::find_if(m_children, [child](const auto& ptr){return ptr.get() == child;});
    itr->reset();
    m_children.erase(itr);

    if(m_keyboard_focus == child)
    {
        m_keyboard_focus = nullptr;
    }

    if(m_mouse_focus == child)
    {
        m_mouse_focus = nullptr;
    }
}

void GuiParent::updateChildren(Engine3D& engine3d)
{
    for(auto& child : m_children)
    {
        child->update(engine3d);
    }
}

void GuiParent::drawChildren(Engine3D& engine3d)
{
    for(auto& child : m_children)
    {
        if(child->isVisible())
        {
            child->draw(engine3d);
        }
    }
}

void GuiParent::setKeyboardFocus(GuiObject* object)
{
    if(object)
    {
        if(m_keyboard_focus)
        {
            if(m_keyboard_focus != object)
            {
                m_keyboard_focus->onLostFocus();
                m_keyboard_focus = object;
                m_keyboard_focus->onGotFocus();
            }
        }
        else
        {
            m_keyboard_focus = object;
            m_keyboard_focus->onGotFocus();
        }
    }
    else
    {
        resetKeyboardFocus();
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
                m_mouse_focus->onCursorExit();
                m_mouse_focus = object;
                m_mouse_focus->onCursorEnter();
            }
        }
        else
        {
            m_mouse_focus = object;
            m_mouse_focus->onCursorEnter();
        }
    }
    else
    {
        resetMouseFocus();
    }
}

void GuiParent::resetKeyboardFocus()
{
    if(m_keyboard_focus)
    {
        m_keyboard_focus->onLostFocus();
        m_keyboard_focus = nullptr;
    }
}

void GuiParent::resetMouseFocus()
{
    if(m_mouse_focus)
    {
        m_mouse_focus->onCursorExit();
        m_mouse_focus = nullptr;
    }
}

void GuiParent::determineKeyboardFocus(GuiObject* object, const InputState& input_state)
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

void GuiParent::determineMouseFocus(GuiObject* object, const InputState& input_state)
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

void GuiParent::updateKeyboardFocus(const InputState& input_state)
{
    setKeyboardFocus(determineKeyboardFocus(input_state));
}

void GuiParent::updateMouseFocus(const InputState& input_state)
{
    setMouseFocus(determineMouseFocus(input_state));
}

void GuiParentObject::update(Engine3D& engine3d)
{
    updateChildren(engine3d);
}

void GuiParentObject::draw(Engine3D& engine3d)
{
    drawChildren(engine3d);
}

void GuiParentObject::onLostFocus()
{
    resetKeyboardFocus();
}

void GuiParentObject::onCursorExit()
{
    resetMouseFocus();
}
