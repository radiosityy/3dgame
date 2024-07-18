#ifndef EVENT_H
#define EVENT_H

#include "platform.h"
#include "geometry.h"

//TODO: rename this file to input_event.h or input.h or something

struct InputState
{
    bool shift() const noexcept
    {
        return keyboard[VKeyLShift] || keyboard[VKeyRShift];
    }

    bool ctrl() const noexcept
    {
        return keyboard[VKeyLCtrl] || keyboard[VKeyRCtrl];
    }

    bool alt() const noexcept
    {
        return keyboard[VKeyLAlt] || keyboard[VKeyRAlt];
    }

    bool meta() const noexcept
    {
        return keyboard[VKeyLMeta] || keyboard[VKeyRMeta];
    }

    bool lmb() const noexcept
    {
        return mouse & LMB;
    }

    std::array<bool, 256> keyboard{};
    MouseButton mouse = 0;
    bool caps_lock = false;
    vec2 cursor_pos;
};

/*
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
*/

#endif // EVENT_H
