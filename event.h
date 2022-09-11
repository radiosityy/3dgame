#ifndef EVENT_H
#define EVENT_H

#include "platform.h"
#include "geometry.h"

struct InputState
{
    bool shift() const
    {
        return keyboard[VKeyRShift] || keyboard[VKeyLShift];
    }

    bool ctrl() const
    {
        return keyboard[VKeyRCtrl] || keyboard[VKeyLCtrl];
    }

    bool alt() const
    {
        return keyboard[VKeyRAlt] || keyboard[VKeyLAlt];
    }

    bool meta() const
    {
        return keyboard[VKeyRMeta] || keyboard[VKeyLMeta];
    }

    std::array<bool, 256> keyboard{};
    MouseButton mouse = 0;
    bool caps_lock = false;
    vec2 cursor_pos;
};

enum class EventType
{
    KeyPressed,
    KeyReleased,
    MousePressed,
    MouseReleased,
    MouseScrolledUp,
    MouseScrolledDown,
    MouseMoved,
    MouseDragged
};

struct Event
{
    bool keyEvent() const
    {
        return (EventType::KeyPressed == event) || (EventType::KeyReleased == event);
    }

    bool mouseEvent() const
    {
        return !keyEvent();
    }

    EventType event;
    MouseButton mouse = 0;
    Key key = 0;
    vec2 cursor_delta= {0, 0};
};

struct ControllerState
{
    std::array<bool, ControllerButtonCount> buttons = {};

    float left_trigger = 0.0f;
    float right_trigger = 0.0f;
    vec2 left_stick = vec2(0.0f, 0.0f);
    vec2 right_stick = vec2(0.0f, 0.0f);
};

enum class ControllerEventType
{
    ButtonPressed,
    ButtonReleased,
    LeftStickMoved,
    RightStickMoved,
    LeftTriggerChange,
    RightTriggerChange
};

struct ControllerEvent
{
    ControllerEventType event;
    uint8_t button = 0;
    float trigger_delta = 0.0f;
    vec2 stick_delta = vec2(0.0f, 0.0f);
};

#endif // EVENT_H
