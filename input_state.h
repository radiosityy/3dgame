#ifndef INPUT_STATE_H
#define INPUT_STATE_H

#include "keycodes.h"
#include "geometry.h"

using MouseButton = uint8_t;
constexpr inline MouseButton LMB = 1;
constexpr inline MouseButton MMB = 2;
constexpr inline MouseButton RMB = 4;

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

    bool mmb() const noexcept
    {
        return mouse & MMB;
    }

    std::array<bool, 256> keyboard = {};
    MouseButton mouse = 0;
    bool caps_lock = false;
    vec2 cursor_pos = {0.0f, 0.0f};
};

#if 0
/*--- controller buttons ---*/
using ControllerButton = uint8_t;

constexpr inline ControllerButton ControllerDPadUp      = 0;
constexpr inline ControllerButton ControllerDPadDown    = 1;
constexpr inline ControllerButton ControllerDPadLeft    = 2;
constexpr inline ControllerButton ControllerDPadRight   = 3;
constexpr inline ControllerButton ControllerStart       = 4;
constexpr inline ControllerButton ControllerBack        = 5;
constexpr inline ControllerButton ControllerLeftThumb   = 6;
constexpr inline ControllerButton ControllerRightThumb  = 7;
constexpr inline ControllerButton ControllerLeftBumper  = 8;
constexpr inline ControllerButton ControllerRightBumper = 9;
constexpr inline ControllerButton ControllerA           = 10;
constexpr inline ControllerButton ControllerB           = 11;
constexpr inline ControllerButton ControllerX           = 12;
constexpr inline ControllerButton ControllerY           = 13;

constexpr inline uint8_t ControllerButtonCount = 14;

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
#endif

#endif // INPUT_STATE_H
