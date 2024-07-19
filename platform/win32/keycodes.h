#ifndef KEYCODES_WIN32_H
#define KEYCODES_WIN32_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>

using Key = uint16_t;

constexpr inline Key VKeyHome = VK_HOME;
constexpr inline Key VKeyEnd = VK_END;
constexpr inline Key VKeyPageUp = VK_PRIOR;
constexpr inline Key VKeyPageDown = VK_NEXT;

constexpr inline Key VKeyUp = VK_UP;
constexpr inline Key VKeyDown = VK_DOWN;
constexpr inline Key VKeyLeft = VK_LEFT;
constexpr inline Key VKeyRight = VK_RIGHT;

constexpr inline Key VKeyLCtrl = VK_LCONTROL;
constexpr inline Key VKeyLAlt = VK_LMENU;
constexpr inline Key VKeyLShift = VK_LSHIFT;
constexpr inline Key VKeyLMeta = VK_LWIN;
constexpr inline Key VKeyRCtrl = VK_RCONTROL;
constexpr inline Key VKeyRAlt = VK_RMENU;
constexpr inline Key VKeyRShift = VK_RSHIFT;
constexpr inline Key VKeyRMeta = VK_RWIN;

constexpr inline Key VKeySpace = VK_SPACE;
constexpr inline Key VKeyCapsLock = VK_CAPITAL;
constexpr inline Key VKeyEsc = VK_ESCAPE;
constexpr inline Key VKeyEnter = VK_RETURN;

constexpr inline Key VKeyMinus = VK_OEM_MINUS;
constexpr inline Key VKeyPlus = VK_OEM_PLUS;
constexpr inline Key VKeyBackspace = VK_BACK;
constexpr inline Key VKeyDelete = VK_DELETE;
constexpr inline Key VKeyTab = VK_TAB;
constexpr inline Key VKeyLBracket = VK_OEM_4;
constexpr inline Key VKeyRBracket = VK_OEM_6;
constexpr inline Key VKeyColon = VK_OEM_1;
constexpr inline Key VKeyQuote = VK_OEM_7;
constexpr inline Key VKeyTidle = VK_OEM_3;
constexpr inline Key VKeyBackslash = VK_OEM_5;
constexpr inline Key VKeyComma = VK_OEM_COMMA;
constexpr inline Key VKeyPeriod = VK_OEM_PERIOD;
constexpr inline Key VKeySlash = VK_OEM_2;

constexpr inline Key VKeyF1 = VK_F1;
constexpr inline Key VKeyF2 = VK_F2;
constexpr inline Key VKeyF3 = VK_F3;
constexpr inline Key VKeyF4 = VK_F4;
constexpr inline Key VKeyF5 = VK_F5;
constexpr inline Key VKeyF6 = VK_F6;
constexpr inline Key VKeyF7 = VK_F7;
constexpr inline Key VKeyF8 = VK_F8;
constexpr inline Key VKeyF9 = VK_F9;
constexpr inline Key VKeyF10 = VK_F10;
constexpr inline Key VKeyF11 = VK_F11;
constexpr inline Key VKeyF12 = VK_F12;

constexpr inline Key VKey1 = 0x31;
constexpr inline Key VKey2 = 0x32;
constexpr inline Key VKey3 = 0x33;
constexpr inline Key VKey4 = 0x34;
constexpr inline Key VKey5 = 0x35;
constexpr inline Key VKey6 = 0x36;
constexpr inline Key VKey7 = 0x37;
constexpr inline Key VKey8 = 0x38;
constexpr inline Key VKey9 = 0x39;
constexpr inline Key VKey0 = 0x30;

constexpr inline Key VKeyA = 0x41;
constexpr inline Key VKeyB = 0x42;
constexpr inline Key VKeyC = 0x43;
constexpr inline Key VKeyD = 0x44;
constexpr inline Key VKeyE = 0x45;
constexpr inline Key VKeyF = 0x46;
constexpr inline Key VKeyG = 0x47;
constexpr inline Key VKeyH = 0x48;
constexpr inline Key VKeyI = 0x49;
constexpr inline Key VKeyJ = 0x4a;
constexpr inline Key VKeyK = 0x4b;
constexpr inline Key VKeyL = 0x4c;
constexpr inline Key VKeyM = 0x4d;
constexpr inline Key VKeyN = 0x4e;
constexpr inline Key VKeyO = 0x4f;
constexpr inline Key VKeyP = 0x50;
constexpr inline Key VKeyQ = 0x51;
constexpr inline Key VKeyR = 0x52;
constexpr inline Key VKeyS = 0x53;
constexpr inline Key VKeyT = 0x54;
constexpr inline Key VKeyU = 0x55;
constexpr inline Key VKeyV = 0x56;
constexpr inline Key VKeyW = 0x57;
constexpr inline Key VKeyX = 0x58;
constexpr inline Key VKeyY = 0x59;
constexpr inline Key VKeyZ = 0x5a;

#endif // KEYCODES_WIN32_H
