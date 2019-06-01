#ifndef RX_INPUT_KEYBOARD_H
#define RX_INPUT_KEYBOARD_H

#include "rx/core/types.h" // rx_f32

namespace rx::input {

struct keyboard_device {
  keyboard_device();

  static constexpr const auto k_keys{384};

  void update(rx_f32 _delta_time);
  void update_key(bool _down, int _scan_code, int _symbol);

  bool is_pressed(int _key, bool _scan_code) const;
  bool is_released(int _key, bool _scan_code) const;
  bool is_held(int _key, bool _scan_code) const;

private:
  enum {
    k_pressed = 1 << 0,
    k_released = 1 << 1,
    k_held = 1 << 2
  };

  int m_symbols[k_keys];
  int m_scan_codes[k_keys];
};

inline bool keyboard_device::is_pressed(int _key, bool _scan_code) const {
  return _scan_code ? (m_scan_codes[_key] & k_pressed) : (m_symbols[_key] & k_pressed);
}

inline bool keyboard_device::is_released(int _key, bool _scan_code) const {
  return _scan_code ? (m_scan_codes[_key] & k_released) : (m_symbols[_key] & k_released);
}

inline bool keyboard_device::is_held(int _key, bool _scan_code) const {
  return _scan_code ? (m_scan_codes[_key] & k_held) : (m_symbols[_key] & k_held);
}

} // namespace rx::input

#endif // RX_INPUT_KEYBOARD_H
