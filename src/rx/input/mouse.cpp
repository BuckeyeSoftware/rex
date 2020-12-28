#include <string.h> // memset

#include "rx/input/mouse.h"

namespace Rx::Input {

Mouse::Mouse() {
  memset(m_buttons, 0, sizeof m_buttons);
}

void Mouse::update(Float32) {
  m_movement = {};
  m_scrolled = false;

  for (Size i = 0; i < k_buttons; i++) {
    m_buttons[i] &= ~(k_pressed | k_released);
  }
}

void Mouse::update_button(bool _down, int _button) {
  if (_down) {
    m_buttons[_button] |= (k_pressed | k_held);
  } else {
    m_buttons[_button] |= k_released;
    m_buttons[_button] &= ~k_held;
  }
}

void Mouse::update_motion(const Math::Vec4i& _movement) {
  m_movement += { _movement.z, _movement.w };
  m_position = { _movement.x, _movement.y };
}

void Mouse::update_scroll(const Math::Vec2i& _scroll) {
  m_scroll = _scroll;
  m_scrolled = true;
}

} // namespace Rx::Input
