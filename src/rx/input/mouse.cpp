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

void mouse_device::update_button(bool down, int button) {
  if (down) {
    m_buttons[button] |= (k_pressed | k_held);
  } else {
    m_buttons[button] |= k_released;
    m_buttons[button] &= ~k_held;
  }
}

void mouse_device::update_motion(const math::vec4i& movement) {
  m_movement += { movement.z, movement.w };
  m_position = { movement.x, movement.y };
}

void mouse_device::update_scroll(const math::vec2i& scroll) {
  m_scroll = scroll;
  m_scrolled = true;
}

} // namespace rx::input
