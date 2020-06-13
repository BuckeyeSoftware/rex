#ifndef RX_INPUT_KEYBOARD_H
#define RX_INPUT_KEYBOARD_H
#include "rx/core/types.h" // rx_f32

namespace Rx::Input {

enum class scan_code : Sint32 {
  k_unknown        = 0,

  k_a              = 4,
  k_b              = 5,
  k_c              = 6,
  k_d              = 7,
  k_e              = 8,
  k_f              = 9,
  k_g              = 10,
  k_h              = 11,
  k_i              = 12,
  k_j              = 13,
  k_k              = 14,
  k_l              = 15,
  k_m              = 16,
  k_n              = 17,
  k_o              = 18,
  k_p              = 19,
  k_q              = 20,
  k_r              = 21,
  k_s              = 22,
  k_t              = 23,
  k_u              = 24,
  k_v              = 25,
  k_w              = 26,
  k_x              = 27,
  k_y              = 28,
  k_z              = 29,

  k_1              = 30,
  k_2              = 31,
  k_3              = 32,
  k_4              = 33,
  k_5              = 34,
  k_6              = 35,
  k_7              = 36,
  k_8              = 37,
  k_9              = 38,
  k_0              = 39,

  k_return         = 40,
  k_escape         = 41,
  k_backspace      = 42,
  k_tab            = 43,
  k_space          = 44,

  k_minus          = 45,
  k_equals         = 46,
  k_left_bracket   = 47,
  k_right_bracket  = 48,
  k_backslash      = 49,

  k_semicolon      = 51,
  k_apostrophe     = 52,
  k_grave          = 53,

  k_comma          = 54,
  k_period         = 55,
  k_slash          = 56,

  k_capslock       = 57,

  k_f1             = 58,
  k_f2             = 59,
  k_f3             = 60,
  k_f4             = 61,
  k_f5             = 62,
  k_f6             = 63,
  k_f7             = 64,
  k_f8             = 65,
  k_f9             = 66,
  k_f10            = 67,
  k_f11            = 68,
  k_f12            = 69,

  k_print_screen   = 70,
  k_scroll_lock    = 71,
  k_pause          = 72,
  k_insert         = 73,

  k_home           = 74,
  k_page_up        = 75,
  k_delete         = 76,
  k_end            = 77,
  k_page_down      = 78,
  k_right          = 79,
  k_left           = 80,
  k_down           = 81,
  k_up             = 82,

  k_kp_divide      = 84,
  k_kp_multiply    = 85,
  k_kp_minus       = 86,
  k_kp_plus        = 87,
  k_kp_enter       = 88,
  k_kp_1           = 89,
  k_kp_2           = 90,
  k_kp_3           = 91,
  k_kp_4           = 92,
  k_kp_5           = 93,
  k_kp_6           = 94,
  k_kp_7           = 95,
  k_kp_8           = 96,
  k_kp_9           = 97,
  k_kp_0           = 98,
  k_kp_period      = 99,

  k_left_control   = 224,
  k_left_shift     = 225,
  k_left_alt       = 226,
  k_left_gui       = 227,
  k_right_control  = 228,
  k_right_shift    = 229,
  k_right_alt      = 230,
  k_right_gui      = 231
};

struct Keyboard {
  Keyboard();

  static constexpr const auto k_keys = 384;

  void update(Float32 _delta_time);
  void update_key(bool _down, int _scan_code, int _symbol);

  bool is_pressed(scan_code _scan_code) const;
  bool is_released(scan_code _scan_code) const;
  bool is_held(scan_code _scan_code) const;

private:
  enum {
    k_pressed  = 1 << 0,
    k_released = 1 << 1,
    k_held     = 1 << 2
  };

  int m_symbols[k_keys];
  int m_scan_codes[k_keys];
};

inline bool Keyboard::is_pressed(scan_code _scan_code) const {
  return m_scan_codes[static_cast<Size>(_scan_code)] & k_pressed;
}
inline bool Keyboard::is_released(scan_code _scan_code) const {
  return m_scan_codes[static_cast<Size>(_scan_code)] & k_released;
}
inline bool Keyboard::is_held(scan_code _scan_code) const {
  return m_scan_codes[static_cast<Size>(_scan_code)] & k_held;
}

} // namespace rx::input

#endif // RX_INPUT_KEYBOARD_H
