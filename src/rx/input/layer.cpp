#include "rx/input/layer.h"
#include "rx/input/context.h"

#include "rx/render/immediate2D.h"

namespace Rx::Input {

Layer::Layer(Context* _context)
  : m_context{_context}
  , m_text{nullptr}
  , m_controllers{_context->allocator()}
  , m_mouse_captured{false}
{
  m_context->append_layer(this);
}

Layer::~Layer() {
  m_context->remove_layer(this);
}

bool Layer::handle_event(const Event& _event) {
  switch (_event.type) {
  case Event::Type::NONE:
    break;
  case Event::Type::KEYBOARD:
    m_keyboard.update_key(_event.as_keyboard.down, _event.as_keyboard.scan_code,
      _event.as_keyboard.symbol);
    break;
  case Event::Type::CONTROLLER_NOTIFICATION:
    switch (_event.as_controller_notification.kind) {
    case ControllerNotificationEvent::Type::CONNECTED:
      if (!m_controllers.resize(_event.as_controller_notification.index)) {
        return false;
      }
      break;
    case ControllerNotificationEvent::Type::DISCONNECTED:
      m_controllers.erase(_event.as_controller_notification.index,
        _event.as_controller_notification.index + 1);
      break;
    }
    break;
  case Event::Type::CONTROLLER_BUTTON:
    m_controllers[_event.as_controller_button.index].update_button(
      _event.as_controller_button.down, _event.as_controller_button.button);
    break;
  case Event::Type::CONTROLLER_MOTION:
    m_controllers[_event.as_controller_motion.index].update_axis(
      _event.as_controller_motion.axis, _event.as_controller_motion.value);
    break;
  case Event::Type::MOUSE_BUTTON:
    m_mouse.update_button(_event.as_mouse_button.down,
      _event.as_mouse_button.button);
    break;
  case Event::Type::MOUSE_SCROLL:
    m_mouse.update_scroll(_event.as_mouse_scroll.value);
    break;
  case Event::Type::MOUSE_MOTION:
    m_mouse.update_motion(_event.as_mouse_motion.value);
    break;
  case Event::Type::TEXT_INPUT:
    if (m_text) {
      m_text->paste(_event.as_text_input.contents);
    }
    break;
  case Event::Type::CLIPBOARD:
    // Should not be possible. The context handles clipboard events globally.
    RX_HINT_UNREACHABLE();
    break;
  }

  return true;
}

void Layer::capture_mouse(bool _capture) {
  m_mouse_captured = _capture;
}

void Layer::capture_text(Text* text_) {
  if (m_text) {
    m_text->m_flags &= ~Text::ACTIVE;
  }

  if (text_) {
    text_->m_flags |= Text::ACTIVE;
    text_->clear();
  }

  m_text = text_;
}

void Layer::raise() {
  m_context->raise_layer(this);
}

bool Layer::is_active() const {
  return &m_context->active_layer() == this;
}

void Layer::update(Float32 _delta_time) {
  // Handle line editing features for the active text.
  if (m_text) {
    if (m_keyboard.is_held(ScanCode::LEFT_CONTROL)
      || m_keyboard.is_held(ScanCode::RIGHT_CONTROL))
    {
      if (m_keyboard.is_pressed(ScanCode::A)) {
        // Control+A = Select All
        m_text->select_all();
      } else if (m_keyboard.is_pressed(ScanCode::C) || m_keyboard.is_pressed(ScanCode::INSERT)) {
        // Control+(C|Insert) = Copy
        if (auto copy = m_text->copy()) {
          m_context->update_clipboard(Utility::move(*copy));
        }
      } else if (m_keyboard.is_pressed(ScanCode::V)) {
        // Control+V = Paste
        m_text->paste(m_context->clipboard());
      } else if (m_keyboard.is_pressed(ScanCode::X)) {
        // Control+X = Cut
        if (auto cut = m_text->cut()) {
          m_context->update_clipboard(Utility::move(*cut));
        }
      }
    }

    if (m_keyboard.is_held(ScanCode::LEFT_SHIFT)
      || m_keyboard.is_held(ScanCode::RIGHT_SHIFT))
    {
      if (m_keyboard.is_pressed(ScanCode::DELETE)) {
        // Shift+Delete = Cut
        if (auto cut = m_text->cut()) {
          m_context->update_clipboard(Utility::move(*cut));
        }
      } else if (m_keyboard.is_pressed(ScanCode::INSERT)) {
        // Shift+Insert = Paste
        m_text->paste(m_context->clipboard());
      } else {
        // While holding SHIFT we're selecting.
        m_text->select(true);
      }
    } else {
      m_text->select(false);
    }

    if (m_keyboard.is_pressed(ScanCode::LEFT)) {
      m_text->move_cursor(Text::Position::LEFT);
    } else if (m_keyboard.is_pressed(ScanCode::RIGHT)) {
      m_text->move_cursor(Text::Position::RIGHT);
    } else if (m_keyboard.is_pressed(ScanCode::HOME)) {
      m_text->move_cursor(Text::Position::HOME);
    } else if (m_keyboard.is_pressed(ScanCode::END)) {
      m_text->move_cursor(Text::Position::END);
    }

    if (m_keyboard.is_pressed(ScanCode::BACKSPACE)) {
      m_text->erase();
    }
  }

  if (m_text) {
    m_text->update(_delta_time);
  }

  m_mouse.update(_delta_time);
  m_keyboard.update(_delta_time);
  m_controllers.each_fwd([_delta_time](Controller& _device) {
    _device.update(_delta_time);
  });
}

void Layer::render_region(Render::Immediate2D* _immediate) const {
  const auto color = is_active()
    ? Math::Vec4f{0.0f, 1.0f, 0.0f, 0.5f}
    : Math::Vec4f{1.0f, 0.0f, 0.0f, 0.5f};

  _immediate->frame_queue().record_rectangle(
    region().offset.cast<Float32>(),
    region().dimensions.cast<Float32>(),
    0.0f,
    color);
}


} // namespace Rx::Input
