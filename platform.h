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

#define PLATFORM_WIN32 1

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <xinput.h>

#define VK_USE_PLATFORM_WIN32_KHR 1
#define VK_PLATFORM_SURFACE_EXTENSION_NAME VK_KHR_WIN32_SURFACE_EXTENSION_NAME

/*--- keyboard keys ---*/
constexpr Key VKeyHome = VK_HOME;
constexpr Key VKeyEnd = VK_END;
constexpr Key VKeyPageUp = VK_PRIOR;
constexpr Key VKeyPageDown = VK_NEXT;

constexpr Key VKeyUp = VK_UP;
constexpr Key VKeyDown = VK_DOWN;
constexpr Key VKeyLeft = VK_LEFT;
constexpr Key VKeyRight = VK_RIGHT;

constexpr Key VKeyLCtrl = VK_LCONTROL;
constexpr Key VKeyLAlt = VK_LMENU;
constexpr Key VKeyLShift = VK_LSHIFT;
constexpr Key VKeyLMeta = VK_LWIN;
constexpr Key VKeyRCtrl = VK_RCONTROL;
constexpr Key VKeyRAlt = VK_RMENU;
constexpr Key VKeyRShift = VK_RSHIFT;
constexpr Key VKeyRMeta = VK_RWIN;

constexpr Key VKeySpace = VK_SPACE;
constexpr Key VKeyCapsLock = VK_CAPITAL;
constexpr Key VKeyEsc = VK_ESCAPE;
constexpr Key VKeyEnter = VK_RETURN;

constexpr Key VKeyMinus = VK_OEM_MINUS;
constexpr Key VKeyPlus = VK_OEM_PLUS;
constexpr Key VKeyBackspace = VK_BACK;
constexpr Key VKeyDelete = VK_DELETE;
constexpr Key VKeyTab = VK_TAB;
constexpr Key VKeyLBracket = VK_OEM_4;
constexpr Key VKeyRBracket = VK_OEM_6;
constexpr Key VKeyColon = VK_OEM_1;
constexpr Key VKeyQuote = VK_OEM_7;
constexpr Key VKeyTidle = VK_OEM_3;
constexpr Key VKeyBackslash = VK_OEM_5;
constexpr Key VKeyComma = VK_OEM_COMMA;
constexpr Key VKeyPeriod = VK_OEM_PERIOD;
constexpr Key VKeySlash = VK_OEM_2;

constexpr Key VKeyF1 = VK_F1;
constexpr Key VKeyF2 = VK_F2;
constexpr Key VKeyF3 = VK_F3;
constexpr Key VKeyF4 = VK_F4;
constexpr Key VKeyF5 = VK_F5;
constexpr Key VKeyF6 = VK_F6;
constexpr Key VKeyF7 = VK_F7;
constexpr Key VKeyF8 = VK_F8;
constexpr Key VKeyF9 = VK_F9;
constexpr Key VKeyF10 = VK_F10;
constexpr Key VKeyF11 = VK_F11;
constexpr Key VKeyF12 = VK_F12;

constexpr Key VKey1 = 0x31;
constexpr Key VKey2 = 0x32;
constexpr Key VKey3 = 0x33;
constexpr Key VKey4 = 0x34;
constexpr Key VKey5 = 0x35;
constexpr Key VKey6 = 0x36;
constexpr Key VKey7 = 0x37;
constexpr Key VKey8 = 0x38;
constexpr Key VKey9 = 0x39;
constexpr Key VKey0 = 0x30;

constexpr Key VKeyA = 0x41;
constexpr Key VKeyB = 0x42;
constexpr Key VKeyC = 0x43;
constexpr Key VKeyD = 0x44;
constexpr Key VKeyE = 0x45;
constexpr Key VKeyF = 0x46;
constexpr Key VKeyG = 0x47;
constexpr Key VKeyH = 0x48;
constexpr Key VKeyI = 0x49;
constexpr Key VKeyJ = 0x4a;
constexpr Key VKeyK = 0x4b;
constexpr Key VKeyL = 0x4c;
constexpr Key VKeyM = 0x4d;
constexpr Key VKeyN = 0x4e;
constexpr Key VKeyO = 0x4f;
constexpr Key VKeyP = 0x50;
constexpr Key VKeyQ = 0x51;
constexpr Key VKeyR = 0x52;
constexpr Key VKeyS = 0x53;
constexpr Key VKeyT = 0x54;
constexpr Key VKeyU = 0x55;
constexpr Key VKeyV = 0x56;
constexpr Key VKeyW = 0x57;
constexpr Key VKeyX = 0x58;
constexpr Key VKeyY = 0x59;
constexpr Key VKeyZ = 0x5a;

struct WindowParameters
{
    HWND hwnd;
    HINSTANCE hinstance;
};

inline void messageBox(std::string_view msg)
{
    MessageBox(NULL, msg.data(), "ERROR", MB_OK);
}

#elif defined(__linux__)

#define PLATFORM_XCB

#define VK_USE_PLATFORM_XCB_KHR 1
#define VK_PLATFORM_SURFACE_EXTENSION_NAME VK_KHR_XCB_SURFACE_EXTENSION_NAME

#include <xcb/xcb.h>
#include <xcb/xcb_image.h>

constexpr Key VKeyHome = 0x6e;
constexpr Key VKeyEnd = 0x73;
constexpr Key VKeyPageUp = 0x70;
constexpr Key VKeyPageDown = 0x75;

constexpr Key VKeyUp = 0x6f;
constexpr Key VKeyDown = 0x74;
constexpr Key VKeyLeft = 0x71;
constexpr Key VKeyRight = 0x72;

constexpr Key VKeyLCtrl = 0x25;
constexpr Key VKeyLAlt = 0x40;
constexpr Key VKeyLShift = 0x32;
constexpr Key VKeyLMeta = 0x85;
constexpr Key VKeyRCtrl = 0x69;
constexpr Key VKeyRAlt = 0x6c;
constexpr Key VKeyRShift = 0x3e;
constexpr Key VKeyRMeta = 0x86;

constexpr Key VKeySpace = 0x41;
constexpr Key VKeyCapsLock = 0x42;
constexpr Key VKeyEsc = 0x09;
constexpr Key VKeyEnter = 0x24;

constexpr Key VKeyMinus = 0x14;
constexpr Key VKeyPlus = 0x15;
constexpr Key VKeyBackspace = 0x16;
constexpr Key VKeyDelete = 0x77;
constexpr Key VKeyTab = 0x17;
constexpr Key VKeyLBracket = 0x22;
constexpr Key VKeyRBracket = 0x23;
constexpr Key VKeyColon = 0x2f;
constexpr Key VKeyQuote = 0x30;
constexpr Key VKeyTidle = 0x31;
constexpr Key VKeyBackslash = 0x33;
constexpr Key VKeyComma = 0x3b;
constexpr Key VKeyPeriod = 0x3c;
constexpr Key VKeySlash = 0x3d;

constexpr Key VKeyF1 = 67;
constexpr Key VKeyF2 = 68;
constexpr Key VKeyF3 = 69;
constexpr Key VKeyF4 = 70;
constexpr Key VKeyF5 = 71;
constexpr Key VKeyF6 = 72;
constexpr Key VKeyF7 = 73;
constexpr Key VKeyF8 = 74;
constexpr Key VKeyF9 = 75;
constexpr Key VKeyF10 = 76;
constexpr Key VKeyF11 = 95;
constexpr Key VKeyF12 = 96;

constexpr Key VKey1 = 0x0a;
constexpr Key VKey2 = 0x0b;
constexpr Key VKey3 = 0x0c;
constexpr Key VKey4 = 0x0d;
constexpr Key VKey5 = 0x0e;
constexpr Key VKey6 = 0x0f;
constexpr Key VKey7 = 0x10;
constexpr Key VKey8 = 0x11;
constexpr Key VKey9 = 0x12;
constexpr Key VKey0 = 0x13;

constexpr Key VKeyA = 0x26;
constexpr Key VKeyB = 0x38;
constexpr Key VKeyC = 0x36;
constexpr Key VKeyD = 0x28;
constexpr Key VKeyE = 0x1a;
constexpr Key VKeyF = 0x29;
constexpr Key VKeyG = 0x2a;
constexpr Key VKeyH = 0x2b;
constexpr Key VKeyI = 0x1f;
constexpr Key VKeyJ = 0x2c;
constexpr Key VKeyK = 0x2d;
constexpr Key VKeyL = 0x2e;
constexpr Key VKeyM = 0x3a;
constexpr Key VKeyN = 0x39;
constexpr Key VKeyO = 0x20;
constexpr Key VKeyP = 0x21;
constexpr Key VKeyQ = 0x18;
constexpr Key VKeyR = 0x1b;
constexpr Key VKeyS = 0x27;
constexpr Key VKeyT = 0x1c;
constexpr Key VKeyU = 0x1e;
constexpr Key VKeyV = 0x37;
constexpr Key VKeyW = 0x19;
constexpr Key VKeyX = 0x35;
constexpr Key VKeyY = 0x1d;
constexpr Key VKeyZ = 0x34;

struct WindowParameters
{
    xcb_connection_t* connection;
    xcb_window_t window;
};

#include <iostream>
inline void messageBox(std::string_view msg)
{
    std::cout << msg << std::endl;
}

#else
#error Unsupported platform
#endif

#endif //PLATFORM_H
