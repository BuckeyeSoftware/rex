#include <string.h> // memset

#include "rx/input/keyboard.h"

namespace Rx::Input {

Keyboard::Keyboard() {
  memset(m_symbols, 0, sizeof m_symbols);
  memset(m_scan_codes, 0, sizeof m_scan_codes);
}

void Keyboard::update(Float32) {
  for (Size i = 0; i < KEYS; i++) {
    m_scan_codes[i] &= ~(PRESSED | RELEASED);
    m_symbols[i] &= ~(PRESSED | RELEASED);
  }
}

void Keyboard::update_key(bool down, int scan_code, int symbol) {
  if (scan_code < KEYS) {
    if (down) {
      m_scan_codes[scan_code] |= (PRESSED | HELD);
    } else {
      m_scan_codes[scan_code] |= RELEASED;
      m_scan_codes[scan_code] &= ~HELD;
    }
  }

  if (symbol < KEYS) {
    if (down) {
      m_symbols[symbol] |= (PRESSED | HELD);
    } else {
      m_symbols[symbol] |= RELEASED;
      m_symbols[symbol] &= ~HELD;
    }
  }
}

} // namespace Rx::Input
