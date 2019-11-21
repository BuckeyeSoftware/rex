#include <SDL.h>
#include "rx/input/input.h"

#include "rx/core/hints/unreachable.h"

namespace rx::input {

void input::update(rx_f32 _delta_time) {
  // Handle line editing features for the active text.
  if (m_active_text) {
    if (m_keyboard.is_held(scan_code::k_left_control)
      || m_keyboard.is_held(scan_code::k_right_control))
    {
      if (m_keyboard.is_pressed(scan_code::k_a)) {
        // Control+A = Select All
        m_active_text->select_all();
      } else if (m_keyboard.is_pressed(scan_code::k_c)) {
        // Control+C = Copy
        m_clipboard = m_active_text->copy();
        SDL_SetClipboardText(m_clipboard.data());
      } else if (m_keyboard.is_pressed(scan_code::k_v)) {
        // Control+V = Paste
        m_active_text->paste(m_clipboard);
      } else if (m_keyboard.is_pressed(scan_code::k_x)) {
        // Control+X = Cut
        m_clipboard = m_active_text->cut();
        SDL_SetClipboardText(m_clipboard.data());
      } else if (m_keyboard.is_pressed(scan_code::k_insert)) {
        // Control+Insert = Copy
        m_clipboard = m_active_text->copy();
        SDL_SetClipboardText(m_clipboard.data());
      }
    }

    if (m_keyboard.is_held(scan_code::k_left_shift)
      || m_keyboard.is_held(scan_code::k_right_shift))
    {
      if (m_keyboard.is_pressed(scan_code::k_delete)) {
        // Shift+Delete = Cut
        m_clipboard = m_active_text->cut();
        SDL_SetClipboardText(m_clipboard.data());
      } else if (m_keyboard.is_pressed(scan_code::k_insert)) {
        // Shift+Insert = Paste
        m_active_text->paste(m_clipboard);
      } else {
        // While holding SHIFT we're selecting.
        m_active_text->select(true);
      }
    } else {
      m_active_text->select(false);
    }

    if (m_keyboard.is_pressed(scan_code::k_left)) {
      m_active_text->move_cursor(text::position::k_left);
    } else if (m_keyboard.is_pressed(scan_code::k_right)) {
      m_active_text->move_cursor(text::position::k_right);
    } else if (m_keyboard.is_pressed(scan_code::k_home)) {
      m_active_text->move_cursor(text::position::k_home);
    } else if (m_keyboard.is_pressed(scan_code::k_end)) {
      m_active_text->move_cursor(text::position::k_end);
    }

    if (m_keyboard.is_pressed(scan_code::k_backspace)) {
      m_active_text->erase();
    }

    m_active_text->update(_delta_time);
  }

  m_mouse.update(_delta_time);
  m_keyboard.update(_delta_time);
  m_controllers.each_fwd([_delta_time](controller_device& _device) {
    _device.update(_delta_time);
  });
}

void input::handle_event(const event& _event) {
  switch (_event.type) {
  case event_type::k_none:
    RX_HINT_UNREACHABLE();
  case event_type::k_keyboard:
    m_keyboard.update_key(_event.as_keyboard.down, _event.as_keyboard.scan_code,
      _event.as_keyboard.symbol);
    break;
  case event_type::k_controller_notification:
    switch (_event.as_controller_notification.kind) {
    case controller_notification_event::type::k_connected:
      m_controllers.resize(_event.as_controller_notification.index);
      break;
    case controller_notification_event::type::k_disconnected:
      m_controllers.erase(_event.as_controller_notification.index,
        _event.as_controller_notification.index + 1);
      break;
    }
    break;
  case event_type::k_controller_button:
    m_controllers[_event.as_controller_button.index].update_button(
      _event.as_controller_button.down, _event.as_controller_button.button);
    break;
  case event_type::k_controller_motion:
    m_controllers[_event.as_controller_motion.index].update_axis(
      _event.as_controller_motion.axis, _event.as_controller_motion.value);
    break;
  case event_type::k_mouse_button:
    m_mouse.update_button(_event.as_mouse_button.down,
      _event.as_mouse_button.button);
    break;
  case event_type::k_mouse_scroll:
    m_mouse.update_scroll(_event.as_mouse_scroll.value);
    break;
  case event_type::k_mouse_motion:
    m_mouse.update_motion(_event.as_mouse_motion.value);
    break;
  case event_type::k_text_input:
    if (m_active_text) {
      m_active_text->paste(_event.as_text_input.contents);
    }
    break;
  case event_type::k_clipboard:
    m_clipboard = utility::move(_event.as_clipboard.contents);
    break;
  }
}

} // namespace rx::input
