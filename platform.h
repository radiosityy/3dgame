#ifndef PLATFORM_H
#define PLATFORM_H

#include <cstdint>
#include <string_view>

using Key = uint16_t;

using MouseButton = uint8_t;
constexpr inline MouseButton LMB = 1;
constexpr inline MouseButton MMB = 2;
constexpr inline MouseButton RMB = 4;

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

#ifdef _WIN32
#include "platform_win32.h"
#elif defined(__linux__)
#include "platform_linux.h"
#else
#error Unsupported platform
#endif

#endif //PLATFORM_H
