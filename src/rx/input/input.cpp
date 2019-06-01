#include "rx/input/input.h"

namespace rx::input {

void input::update(rx_f32 _delta_time) {
  m_mouse.update(_delta_time);
  m_keyboard.update(_delta_time);
}

void input::handle_event(event&& _event) {
  switch (_event.type) {
  case event_type::k_none:
    RX_UNREACHABLE();
  case event_type::k_keyboard:
    m_keyboard.update_key(
      _event.as_keyboard.down,
      _event.as_keyboard.scan_code,
      _event.as_keyboard.symbol);
    break;
  case event_type::k_mouse_button:
    m_mouse.update_button(
      _event.as_mouse_button.down,
      _event.as_mouse_button.button);
    break;
  case event_type::k_mouse_scroll:
    m_mouse.update_scroll(_event.as_mouse_scroll.value);
    break;
  case event_type::k_mouse_motion:
    m_mouse.update_motion(_event.as_mouse_motion.value);
    break;
  }
}

} // namespace rx::input
