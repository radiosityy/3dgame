#ifndef GUI_OBJECT_H
#define GUI_OBJECT_H

#include "input_state.h"
#include "render_data.h"
#include "geometry.h"
#include "engine_3d.h"

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
    virtual void onMouseReleased(MouseButton, const InputState&);
    virtual void onMouseMoved(vec2, const InputState&);
    virtual void onMouseDragged(vec2, const InputState&);
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

    virtual void update(Engine3D& engine3d);
    virtual void draw(Engine3D& engine3d) = 0;

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
    virtual void onMouseReleased(MouseButton, const InputState&) final;
    virtual void onMouseMoved(vec2, const InputState&) final;
    virtual void onMouseDragged(vec2, const InputState&) final;
    virtual void onMouseScrolledUp(const InputState&) final;
    virtual void onMouseScrolledDown(const InputState&) final;

protected:
    virtual bool onKeyPressedIntercept(Key, const InputState&);
    virtual bool onKeyReleasedIntercept(Key, const InputState&);
    virtual bool onMousePressedIntercept(MouseButton, const InputState&);
    virtual bool onMouseReleasedIntercept(MouseButton, const InputState&);
    virtual bool onMouseMovedIntercept(vec2, const InputState&);
    virtual bool onMouseDraggedIntercept(vec2, const InputState&);
    virtual bool onMouseScrolledUpIntercept(const InputState&);
    virtual bool onMouseScrolledDownIntercept(const InputState&);

    virtual void onKeyPressedImpl(Key, const InputState&);
    virtual void onKeyReleasedImpl(Key, const InputState&);
    virtual void onMousePressedImpl(MouseButton, const InputState&);
    virtual void onMouseReleasedImpl(MouseButton, const InputState&);
    virtual void onMouseMovedImpl(vec2, const InputState&);
    virtual void onMouseDraggedImpl(vec2, const InputState&);
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
    void updateChildren(Engine3D&);
    void drawChildren(Engine3D&);

    void setKeyboardFocus(GuiObject*);
    void setMouseFocus(GuiObject*);
    void resetKeyboardFocus();
    void resetMouseFocus();
    void determineKeyboardFocus(GuiObject*, const InputState&);
    void determineMouseFocus(GuiObject*, const InputState&);

    GuiObject* m_keyboard_focus = nullptr;
    GuiObject* m_mouse_focus = nullptr;

private:
    std::vector<std::unique_ptr<GuiObject>> m_children;

    void determineKeyboardFocus(const InputState& input_state)
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

        setKeyboardFocus(object_under_cursor);
    }

    void determineMouseFocus(const InputState& input_state)
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

        setMouseFocus(object_under_cursor);
    }
};

class GuiParentObject : public GuiObject, public GuiParent
{
public:
    virtual void update(Engine3D&) override;
    virtual void draw(Engine3D&) override;

    virtual void onLostFocus() override;
    virtual void onCursorExit() override;
};

#endif // GUI_OBJECT_H
