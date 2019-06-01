#include <string.h> // memset

#include "rx/input/mouse.h"

namespace rx::input {

mouse_device::mouse_device() {
  memset(m_buttons, 0, sizeof m_buttons);
}

void mouse_device::update(rx_f32) {
  m_movement = {};
  m_scrolled = false;

  for (rx_size i{0}; i < k_buttons; i++) {
    m_buttons[i] &= ~(k_pressed | k_released);
  }
}

void mouse_device::update_button(bool _down, int _button) {
  if (_down) {
    m_buttons[_button] |= (k_pressed | k_held);
  } else {
    m_buttons[_button] |= k_released;
    m_buttons[_button] &= ~k_held;
  }
}

void mouse_device::update_motion(const math::vec4i& _movement) {
  m_movement += { _movement.z, _movement.w };
  m_position = { _movement.x, _movement.y };
}

void mouse_device::update_scroll(const math::vec2i& _scroll) {
  m_scroll = _scroll;
  m_scrolled = true;
}

} // namespace rx::input
