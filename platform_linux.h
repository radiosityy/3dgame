#ifndef PLATFORM_LINUX_H
#define PLATFORM_LINUX_H

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

#include <print>
inline void messageBox(std::string_view msg)
{
    std::println("{}", msg);
}

#endif //PLATFORM_LINUX_H
