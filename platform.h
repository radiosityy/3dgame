#ifndef PLATFORM_H
#define PLATFORM_H

#include <cstdint>
#include <string_view>

using Key = uint16_t;

using MouseButton = uint8_t;
constexpr MouseButton LMB = 1;
constexpr MouseButton MMB = 2;
constexpr MouseButton RMB = 4;

using ControllerButton = uint8_t;

/*--- controller buttons ---*/
constexpr ControllerButton ControllerDPadUp      = 0;
constexpr ControllerButton ControllerDPadDown    = 1;
constexpr ControllerButton ControllerDPadLeft    = 2;
constexpr ControllerButton ControllerDPadRight   = 3;
constexpr ControllerButton ControllerStart       = 4;
constexpr ControllerButton ControllerBack        = 5;
constexpr ControllerButton ControllerLeftThumb   = 6;
constexpr ControllerButton ControllerRightThumb  = 7;
constexpr ControllerButton ControllerLeftBumper  = 8;
constexpr ControllerButton ControllerRightBumper = 9;
constexpr ControllerButton ControllerA           = 10;
constexpr ControllerButton ControllerB           = 11;
constexpr ControllerButton ControllerX           = 12;
constexpr ControllerButton ControllerY           = 13;

constexpr uint8_t ControllerButtonCount = 14;

#ifdef _WIN32
#include "platform_win32.h"
#elif defined(__linux__)
#include "platform_linux.h"
#else
#error Unsupported platform
#endif

#endif //PLATFORM_H
