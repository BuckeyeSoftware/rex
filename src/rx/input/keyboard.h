#ifndef RX_INPUT_KEYBOARD_H
#define RX_INPUT_KEYBOARD_H
#include "rx/core/types.h" // Float32, Sint32

namespace Rx::Input {

enum class ScanCode : Sint32 {
  UNKNOWN        = 0,

  A              = 4,
  B              = 5,
  C              = 6,
  D              = 7,
  E              = 8,
  F              = 9,
  G              = 10,
  H              = 11,
  I              = 12,
  J              = 13,
  K              = 14,
  L              = 15,
  M              = 16,
  N              = 17,
  O              = 18,
  P              = 19,
  Q              = 20,
  R              = 21,
  S              = 22,
  T              = 23,
  U              = 24,
  V              = 25,
  W              = 26,
  X              = 27,
  Y              = 28,
  Z              = 29,

  N1             = 30,
  N2             = 31,
  N3             = 32,
  N4             = 33,
  N5             = 34,
  N6             = 35,
  N7             = 36,
  N8             = 37,
  N9             = 38,
  N0             = 39,

  RETURN         = 40,
  ESCAPE         = 41,
  BACKSPACE      = 42,
  TAB            = 43,
  SPACE          = 44,

  MINUS          = 45,
  EQUALS         = 46,
  LEFT_BRACKET   = 47,
  RIGHT_BRACKET  = 48,
  BACKSLASH      = 49,

  SEMICOLON      = 51,
  APOSTROPHE     = 52,
  GRAVE          = 53,

  COMMA          = 54,
  PERIOD         = 55,
  SLASH          = 56,

  CAPSLOCK       = 57,

  F1             = 58,
  F2             = 59,
  F3             = 60,
  F4             = 61,
  F5             = 62,
  F6             = 63,
  F7             = 64,
  F8             = 65,
  F9             = 66,
  F10            = 67,
  F11            = 68,
  F12            = 69,

  PRINT_SCREEN   = 70,
  SCROLL_LOCK    = 71,
  PAUSE          = 72,
  INSERT         = 73,

  HOME           = 74,
  PAGE_UP        = 75,
  DELETE         = 76,
  END            = 77,
  PAGE_DOWN      = 78,
  RIGHT          = 79,
  LEFT           = 80,
  DOWN           = 81,
  UP             = 82,

  KP_DIVIDE      = 84,
  KP_MULTIPLY    = 85,
  KP_MINUS       = 86,
  KP_PLUS        = 87,
  KP_ENTER       = 88,
  KP_1           = 89,
  KP_2           = 90,
  KP_3           = 91,
  KP_4           = 92,
  KP_5           = 93,
  KP_6           = 94,
  KP_7           = 95,
  KP_8           = 96,
  KP_9           = 97,
  KP_0           = 98,
  KP_PERIOD      = 99,

  LEFT_CONTROL   = 224,
  LEFT_SHIFT     = 225,
  LEFT_ALT       = 226,
  LEFT_GUI       = 227,
  RIGHT_CONTROL  = 228,
  RIGHT_SHIFT    = 229,
  RIGHT_ALT      = 230,
  RIGHT_GUI      = 231
};

struct Keyboard {
  Keyboard();

  static inline constexpr const auto KEYS = 384;

  void update(Float32 _delta_time);
  void update_key(bool _down, int _scan_code, int _symbol);

  bool is_pressed(ScanCode _scan_code) const;
  bool is_released(ScanCode _scan_code) const;
  bool is_held(ScanCode _scan_code) const;

private:
  enum : Uint8 {
    PRESSED  = 1 << 0,
    RELEASED = 1 << 1,
    HELD     = 1 << 2
  };

  int m_symbols[KEYS];
  int m_scan_codes[KEYS];
};

inline bool Keyboard::is_pressed(ScanCode _scan_code) const {
  return m_scan_codes[static_cast<Size>(_scan_code)] & PRESSED;
}
inline bool Keyboard::is_released(ScanCode _scan_code) const {
  return m_scan_codes[static_cast<Size>(_scan_code)] & RELEASED;
}
inline bool Keyboard::is_held(ScanCode _scan_code) const {
  return m_scan_codes[static_cast<Size>(_scan_code)] & HELD;
}

} // namespace Rx::Input

#endif // RX_INPUT_KEYBOARD_H
