#include <string.h> // memset

#include <rx/input/mouse.h>

namespace rx::input {

mouse::mouse() {
  memset(m_buttons, 0, sizeof m_buttons);
}

void mouse::update(rx_f32) {
  m_movement = {};
  m_scrolled = false;

  for (rx_size i{0}; i < k_buttons; i++) {
    m_buttons[i] &= ~(k_pressed | k_released);
  }
}

void mouse::update_button(bool down, int button) {
  if (down) {
    m_buttons[button] |= (k_pressed | k_held);
  } else {
    m_buttons[button] |= k_released;
    m_buttons[button] &= ~k_held;
  }
}

void mouse::update_motion(const math::vec2i& movement) {
  m_movement = movement - m_position;
  m_position = movement;
}

void mouse::update_scroll(const math::vec2i& scroll) {
  m_scroll = scroll;
  m_scrolled = true;
}

} // namespace rx::input
