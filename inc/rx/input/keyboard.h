#ifndef RX_INPUT_KEYBOARD_H
#define RX_INPUT_KEYBOARD_H

#include <rx/core/types.h> // rx_f32

namespace rx::input {

struct keyboard {
  keyboard();

  static constexpr const auto k_keys{384};

  void update(rx_f32 delta_time);
  void update_key(bool down, int scan_code, int symbol);

  bool is_pressed(int key, bool scan_code) const;
  bool is_released(int key, bool scan_code) const;
  bool is_held(int key, bool scan_code) const;

private:
  enum {
    k_pressed = 1 << 0,
    k_released = 1 << 1,
    k_held = 1 << 2
  };

  int m_symbols[k_keys];
  int m_scan_codes[k_keys];
};

inline bool keyboard::is_pressed(int key, bool scan_code) const {
  return scan_code ? (m_scan_codes[key] & k_pressed) : (m_symbols[key] & k_pressed);
}

inline bool keyboard::is_released(int key, bool scan_code) const {
  return scan_code ? (m_scan_codes[key] & k_released) : (m_symbols[key] & k_released);
}

inline bool keyboard::is_held(int key, bool scan_code) const {
  return scan_code ? (m_scan_codes[key] & k_held) : (m_symbols[key] & k_held);
}

} // namespace rx::input

#endif // RX_INPUT_KEYBOARD_H
