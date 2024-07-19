#ifndef KEYCODES_XCB_H
#define KEYCODES_XCB_H

#include <cstdint>

using Key = uint16_t;

constexpr inline Key VKeyHome = 0x6e;
constexpr inline Key VKeyEnd = 0x73;
constexpr inline Key VKeyPageUp = 0x70;
constexpr inline Key VKeyPageDown = 0x75;

constexpr inline Key VKeyUp = 0x6f;
constexpr inline Key VKeyDown = 0x74;
constexpr inline Key VKeyLeft = 0x71;
constexpr inline Key VKeyRight = 0x72;

constexpr inline Key VKeyLCtrl = 0x25;
constexpr inline Key VKeyLAlt = 0x40;
constexpr inline Key VKeyLShift = 0x32;
constexpr inline Key VKeyLMeta = 0x85;
constexpr inline Key VKeyRCtrl = 0x69;
constexpr inline Key VKeyRAlt = 0x6c;
constexpr inline Key VKeyRShift = 0x3e;
constexpr inline Key VKeyRMeta = 0x86;

constexpr inline Key VKeySpace = 0x41;
constexpr inline Key VKeyCapsLock = 0x42;
constexpr inline Key VKeyEsc = 0x09;
constexpr inline Key VKeyEnter = 0x24;

constexpr inline Key VKeyMinus = 0x14;
constexpr inline Key VKeyPlus = 0x15;
constexpr inline Key VKeyBackspace = 0x16;
constexpr inline Key VKeyDelete = 0x77;
constexpr inline Key VKeyTab = 0x17;
constexpr inline Key VKeyLBracket = 0x22;
constexpr inline Key VKeyRBracket = 0x23;
constexpr inline Key VKeyColon = 0x2f;
constexpr inline Key VKeyQuote = 0x30;
constexpr inline Key VKeyTidle = 0x31;
constexpr inline Key VKeyBackslash = 0x33;
constexpr inline Key VKeyComma = 0x3b;
constexpr inline Key VKeyPeriod = 0x3c;
constexpr inline Key VKeySlash = 0x3d;

constexpr inline Key VKeyF1 = 67;
constexpr inline Key VKeyF2 = 68;
constexpr inline Key VKeyF3 = 69;
constexpr inline Key VKeyF4 = 70;
constexpr inline Key VKeyF5 = 71;
constexpr inline Key VKeyF6 = 72;
constexpr inline Key VKeyF7 = 73;
constexpr inline Key VKeyF8 = 74;
constexpr inline Key VKeyF9 = 75;
constexpr inline Key VKeyF10 = 76;
constexpr inline Key VKeyF11 = 95;
constexpr inline Key VKeyF12 = 96;

constexpr inline Key VKey1 = 0x0a;
constexpr inline Key VKey2 = 0x0b;
constexpr inline Key VKey3 = 0x0c;
constexpr inline Key VKey4 = 0x0d;
constexpr inline Key VKey5 = 0x0e;
constexpr inline Key VKey6 = 0x0f;
constexpr inline Key VKey7 = 0x10;
constexpr inline Key VKey8 = 0x11;
constexpr inline Key VKey9 = 0x12;
constexpr inline Key VKey0 = 0x13;

constexpr inline Key VKeyA = 0x26;
constexpr inline Key VKeyB = 0x38;
constexpr inline Key VKeyC = 0x36;
constexpr inline Key VKeyD = 0x28;
constexpr inline Key VKeyE = 0x1a;
constexpr inline Key VKeyF = 0x29;
constexpr inline Key VKeyG = 0x2a;
constexpr inline Key VKeyH = 0x2b;
constexpr inline Key VKeyI = 0x1f;
constexpr inline Key VKeyJ = 0x2c;
constexpr inline Key VKeyK = 0x2d;
constexpr inline Key VKeyL = 0x2e;
constexpr inline Key VKeyM = 0x3a;
constexpr inline Key VKeyN = 0x39;
constexpr inline Key VKeyO = 0x20;
constexpr inline Key VKeyP = 0x21;
constexpr inline Key VKeyQ = 0x18;
constexpr inline Key VKeyR = 0x1b;
constexpr inline Key VKeyS = 0x27;
constexpr inline Key VKeyT = 0x1c;
constexpr inline Key VKeyU = 0x1e;
constexpr inline Key VKeyV = 0x37;
constexpr inline Key VKeyW = 0x19;
constexpr inline Key VKeyX = 0x35;
constexpr inline Key VKeyY = 0x1d;
constexpr inline Key VKeyZ = 0x34;

#endif //KEYCODES_XCB_H
