#include "rx/input/context.h"

#include "rx/core/hints/unreachable.h"

namespace Rx::Input {

int Context::update(Float32 _delta_time) {
  // Handle line editing features for the active text.
  if (m_active_text) {
    if (m_keyboard.is_held(ScanCode::k_left_control)
      || m_keyboard.is_held(ScanCode::k_right_control))
    {
      if (m_keyboard.is_pressed(ScanCode::k_a)) {
        // Control+A = Select All
        m_active_text->select_all();
      } else if (m_keyboard.is_pressed(ScanCode::k_c)) {
        // Control+C = Copy
        change_clipboard_text(m_active_text->copy());
      } else if (m_keyboard.is_pressed(ScanCode::k_v)) {
        // Control+V = Paste
        m_active_text->paste(m_clipboard);
      } else if (m_keyboard.is_pressed(ScanCode::k_x)) {
        // Control+X = Cut
        change_clipboard_text(m_active_text->cut());
      } else if (m_keyboard.is_pressed(ScanCode::k_insert)) {
        // Control+Insert = Copy
        change_clipboard_text(m_active_text->copy());
      }
    }

    if (m_keyboard.is_held(ScanCode::k_left_shift)
      || m_keyboard.is_held(ScanCode::k_right_shift))
    {
      if (m_keyboard.is_pressed(ScanCode::k_delete)) {
        // Shift+Delete = Cut
        change_clipboard_text(m_active_text->cut());
      } else if (m_keyboard.is_pressed(ScanCode::k_insert)) {
        // Shift+Insert = Paste
        m_active_text->paste(m_clipboard);
      } else {
        // While holding SHIFT we're selecting.
        m_active_text->select(true);
      }
    } else {
      m_active_text->select(false);
    }

    if (m_keyboard.is_pressed(ScanCode::k_left)) {
      m_active_text->move_cursor(Text::Position::k_left);
    } else if (m_keyboard.is_pressed(ScanCode::k_right)) {
      m_active_text->move_cursor(Text::Position::k_right);
    } else if (m_keyboard.is_pressed(ScanCode::k_home)) {
      m_active_text->move_cursor(Text::Position::k_home);
    } else if (m_keyboard.is_pressed(ScanCode::k_end)) {
      m_active_text->move_cursor(Text::Position::k_end);
    }

    if (m_keyboard.is_pressed(ScanCode::k_backspace)) {
      m_active_text->erase();
    }
  }

  int updated{m_updated};

  if (m_active_text) {
    m_active_text->update(_delta_time);
  }

  m_mouse.update(_delta_time);
  m_keyboard.update(_delta_time);
  m_controllers.each_fwd([_delta_time](Controller& _device) {
    _device.update(_delta_time);
  });

  m_updated = 0;

  return updated;
}

void Context::handle_event(const Event& _event) {
  switch (_event.type) {
  case Event::Type::k_none:
    break;
  case Event::Type::k_keyboard:
    m_keyboard.update_key(_event.as_keyboard.down, _event.as_keyboard.scan_code,
      _event.as_keyboard.symbol);
    break;
  case Event::Type::k_controller_notification:
    switch (_event.as_controller_notification.kind) {
    case ControllerNotificationEvent::Type::k_connected:
      m_controllers.resize(_event.as_controller_notification.index);
      break;
    case ControllerNotificationEvent::Type::k_disconnected:
      m_controllers.erase(_event.as_controller_notification.index,
        _event.as_controller_notification.index + 1);
      break;
    }
    break;
  case Event::Type::k_controller_button:
    m_controllers[_event.as_controller_button.index].update_button(
      _event.as_controller_button.down, _event.as_controller_button.button);
    break;
  case Event::Type::k_controller_motion:
    m_controllers[_event.as_controller_motion.index].update_axis(
      _event.as_controller_motion.axis, _event.as_controller_motion.value);
    break;
  case Event::Type::k_mouse_button:
    m_mouse.update_button(_event.as_mouse_button.down,
      _event.as_mouse_button.button);
    break;
  case Event::Type::k_mouse_scroll:
    m_mouse.update_scroll(_event.as_mouse_scroll.value);
    break;
  case Event::Type::k_mouse_motion:
    m_mouse.update_motion(_event.as_mouse_motion.value);
    break;
  case Event::Type::k_text_input:
    if (m_active_text) {
      m_active_text->paste(_event.as_text_input.contents);
    }
    break;
  case Event::Type::k_clipboard:
    m_clipboard = Utility::move(_event.as_clipboard.contents);
    break;
  }
}

} // namespace rx::input
