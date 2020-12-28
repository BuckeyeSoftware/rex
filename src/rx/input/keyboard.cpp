#include <string.h> // memset

#include "rx/input/keyboard.h"

namespace Rx::Input {

Keyboard::Keyboard() {
  memset(m_symbols, 0, sizeof m_symbols);
  memset(m_scan_codes, 0, sizeof m_scan_codes);
}

void Keyboard::update(Float32) {
  for (Size i{0}; i < k_keys; i++) {
    m_scan_codes[i] &= ~(k_pressed | k_released);
    m_symbols[i] &= ~(k_pressed | k_released);
  }
}

void Keyboard::update_key(bool down, int scan_code, int symbol) {
  if (scan_code < k_keys) {
    if (down) {
      m_scan_codes[scan_code] |= (k_pressed | k_held);
    } else {
      m_scan_codes[scan_code] |= k_released;
      m_scan_codes[scan_code] &= ~k_held;
    }
  }

  if (symbol < k_keys) {
    if (down) {
      m_symbols[symbol] |= (k_pressed | k_held);
    } else {
      m_symbols[symbol] |= k_released;
      m_symbols[symbol] &= ~k_held;
    }
  }
}

} // namespace Rx::Input
