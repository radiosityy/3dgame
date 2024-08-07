#ifndef GUI_OBJECT_H
#define GUI_OBJECT_H

#include "input_state.h"
#include "render_data.h"
#include "geometry.h"
#include "renderer.h"

#include <memory>

enum class HorizontalAlignment {Left, Center, Right};
enum class VerticalAlignment {Top, Center, Bottom};

struct TexId
{
    TexId() = default;
    TexId(uint32_t _tex_id, uint32_t _layer_id)
        : tex_id(_tex_id)
        , layer_id(_layer_id)
    {}

    uint32_t tex_id;
    uint32_t layer_id;
};

class InputHandler
{
public:
    virtual void onKeyPressed(Key, const InputState&);
    virtual void onKeyReleased(Key, const InputState&);
    virtual void onMousePressed(MouseButton, const InputState&);
    virtual void onMouseReleased(MouseButton, const InputState&, bool released_inside);
    virtual void onMouseMoved(vec2, const InputState&);
    virtual void onMouseScrolledUp(const InputState&);
    virtual void onMouseScrolledDown(const InputState&);
};

class GuiObject : public virtual InputHandler
{
public:
    virtual ~GuiObject() = default;

    virtual void onGotFocus();
    virtual void onLostFocus();
    virtual void onCursorEnter();
    virtual void onCursorExit();

    virtual void update(Renderer& renderer);
    virtual void draw(Renderer& renderer) = 0;

    virtual void move(vec2);
    virtual void scale(vec2);

    virtual bool isPointInside(vec2) = 0;
    virtual void setScissor(Quad scissor) = 0;

    bool isVisible() const noexcept;
    void setVisible(bool) noexcept;

    bool isFocusable() const noexcept;
    void setFocusable(bool) noexcept;

protected:
    bool m_visible = true;
    bool m_focusable = true;
};

class GuiParent : public virtual InputHandler
{
public:
    virtual void onKeyPressed(Key, const InputState&) final;
    virtual void onKeyReleased(Key, const InputState&) final;
    virtual void onMousePressed(MouseButton, const InputState&) final;
    virtual void onMouseReleased(MouseButton, const InputState&, bool released_inside) final;
    virtual void onMouseMoved(vec2, const InputState&) final;
    virtual void onMouseScrolledUp(const InputState&) final;
    virtual void onMouseScrolledDown(const InputState&) final;

protected:
    virtual bool onKeyPressedIntercept(Key, const InputState&);
    virtual bool onKeyReleasedIntercept(Key, const InputState&);
    virtual bool onMousePressedIntercept(MouseButton, const InputState&);
    virtual bool onMouseReleasedIntercept(MouseButton, const InputState&, bool released_inside);
    virtual bool onMouseMovedIntercept(vec2, const InputState&);
    virtual bool onMouseScrolledUpIntercept(const InputState&);
    virtual bool onMouseScrolledDownIntercept(const InputState&);

    virtual void onKeyPressedImpl(Key, const InputState&);
    virtual void onKeyReleasedImpl(Key, const InputState&);
    virtual void onMousePressedImpl(MouseButton, const InputState&);
    virtual void onMouseReleasedImpl(MouseButton, const InputState&, bool released_inside);
    virtual void onMouseMovedImpl(vec2, const InputState&);
    virtual void onMouseScrolledUpImpl(const InputState&);
    virtual void onMouseScrolledDownImpl(const InputState&);

    template<class T>
    T* addChild(std::unique_ptr<T>&& obj)
    {
        T* p = obj.get();
        m_children.emplace_back(std::move(obj));
        return p;
    }
    void removeChild(GuiObject*);
    void updateChildren(Renderer&);
    void drawChildren(Renderer&);

    void setKeyboardFocus(GuiObject*);
    void setKeyboardFocusRedirect(GuiObject*);
    void setMouseFocus(GuiObject*);
    void resetKeyboardFocus();
    void resetMouseFocus();
    GuiObject* m_keyboard_focus = nullptr;
    GuiObject* m_mouse_focus = nullptr;

private:
    std::vector<std::unique_ptr<GuiObject>> m_children;
    GuiObject* m_keyboard_focus_redirect = nullptr;

    GuiObject* determineKeyboardFocus(const InputState& input_state)
    {
        GuiObject* object_under_cursor = nullptr;

        for(auto itr = m_children.crbegin(); itr != m_children.crend(); itr++)
        {
            GuiObject* object = itr->get();

            if(object->isVisible() && object->isPointInside(input_state.cursor_pos))
            {
                if(object->isFocusable())
                {
                    object_under_cursor = object;
                }
                else
                {
                    object_under_cursor = nullptr;
                }

                break;
            }
        }

        if(!object_under_cursor && m_keyboard_focus_redirect)
        {
            return m_keyboard_focus_redirect;
        }
        else
        {
            return object_under_cursor;
        }
    }

    GuiObject* determineMouseFocus(const InputState& input_state)
    {
        GuiObject* object_under_cursor = nullptr;

        for(auto itr = m_children.crbegin(); itr != m_children.crend(); itr++)
        {
            GuiObject* object = itr->get();

            if(object->isVisible() && object->isPointInside(input_state.cursor_pos))
            {
                object_under_cursor = object;
                break;
            }
        }

        return object_under_cursor;
    }

    void updateKeyboardFocus(const InputState&);
    void updateMouseFocus(const InputState&);
};

class GuiParentObject : public GuiObject, public GuiParent
{
public:
    virtual void update(Renderer&) override;
    virtual void draw(Renderer&) override;

    virtual void onLostFocus() override;
    virtual void onCursorExit() override;
};

#endif // GUI_OBJECT_H
