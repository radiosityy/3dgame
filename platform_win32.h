#ifndef PLATFORM_WIN32_H
#define PLATFORM_WIN32_H

#define PLATFORM_WIN32 1

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
//#include <xinput.h>

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
#endif // PLATFORM_WIN32_H
