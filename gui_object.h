#ifndef GUI_OBJECT_H
#define GUI_OBJECT_H

#include "event.h"
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

class GuiObject
{
public:
    virtual ~GuiObject() = default;

    virtual void onInputEvent(const Event& event, const InputState& input_state);

    virtual void gotFocus();
    virtual void lostFocus();
    virtual void cursorEnter();
    virtual void cursorExit();

    virtual void update(Engine3D& engine3d, float dt) = 0;
    virtual void draw(Engine3D& engine3d) = 0;
    void updateAndDraw(Engine3D& engine3d, float dt);

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

class GuiParent
{
protected:
    void setKeyboardFocus(GuiObject*);
    void setMouseFocus(GuiObject*);
    void resetKeyboardFocus();
    void resetMouseFocus();
    bool forwardEventToFocused(const Event&, const InputState&);
    void determineKeyboardFocus(GuiObject*, const Event&, const InputState&);
    void determineMouseFocus(GuiObject*, const Event&, const InputState&);

    template<class Ptr>
    void determineKeyboardFocus(const std::vector<Ptr>& objects, const Event& event, const InputState& input_state)
    {
        /*on LMB click determine keyboard focus*/
        if((EventType::MousePressed == event.event) && (LMB == event.mouse))
        {
            GuiObject* object_under_cursor = nullptr;

            for(auto itr = objects.crbegin(); itr != objects.crend(); itr++)
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
    }

    template<class Ptr>
    void determineMouseFocus(const std::vector<Ptr>& objects, const Event& event, const InputState& input_state)
    {
        /*on mouse move determine mouse focus*/
        if(EventType::MouseMoved == event.event)
        {
            GuiObject* object_under_cursor = nullptr;

            for(auto itr = objects.crbegin(); itr != objects.crend(); itr++)
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
    }

    template<class Ptr>
    void determineFocus(const std::vector<Ptr>& objects, const Event& event, const InputState& input_state)
    {
        determineKeyboardFocus(objects, event, input_state);
        determineMouseFocus(objects, event, input_state);
    }

    GuiObject* m_keyboard_focus = nullptr;
    GuiObject* m_mouse_focus = nullptr;
};

class GuiParentObject : public GuiObject, public GuiParent
{
public:
    virtual void lostFocus() override;
    virtual void cursorExit() override;
};

#endif // GUI_OBJECT_H
