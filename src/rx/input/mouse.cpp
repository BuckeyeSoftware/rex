#include "rx/input/mouse.h"
#include "rx/core/memory/zero.h"

namespace Rx::Input {

Mouse::Mouse() {
  Memory::zero(m_buttons);
}

void Mouse::update(Float32) {
  m_movement = {};
  m_scrolled = false;

  for (Size i = 0; i < BUTTONS; i++) {
    m_buttons[i] &= ~(PRESSED | RELEASED);
  }
}

void Mouse::update_button(bool _down, int _button) {
  if (_down) {
    m_buttons[_button] |= (PRESSED | HELD);
  } else {
    m_buttons[_button] |= RELEASED;
    m_buttons[_button] &= ~HELD;
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
