#ifndef RX_INPUT_INPUT_H
#define RX_INPUT_INPUT_H
#include "rx/core/vector.h"

#include "rx/input/mouse.h"
#include "rx/input/keyboard.h"
#include "rx/input/event.h"
#include "rx/input/controller.h"
#include "rx/input/text.h"

namespace Rx::Input {

struct Context {
  enum {
    k_clipboard     = 1 << 0,
    k_mouse_capture = 1 << 1
  };

  Context();
  Context(Memory::Allocator& _allocator);

  void handle_event(const Event& _event);
  int update(Float32 _delta_time);

  const Mouse& mouse() const &;
  const Keyboard& keyboard() const &;
  const Vector<Controller>& controllers() const &;
  const String& clipboard() const &;

  Text* active_text() const;

  bool is_mouse_captured() const;

  void capture_mouse(bool _capture);
  void capture_text(Text* text_);

private:
  void change_clipboard_text(String&& text_);

  Mouse m_mouse;
  Keyboard m_keyboard;
  Vector<Controller> m_controllers;
  Text* m_active_text;
  String m_clipboard;
  bool m_mouse_captured;
  int m_updated;
};

inline Context::Context()
  : Context{Memory::SystemAllocator::instance()}
{
}

inline Context::Context(Memory::Allocator& _allocator)
  : m_controllers{_allocator}
  , m_active_text{nullptr}
  , m_clipboard{_allocator}
  , m_mouse_captured{true}
  , m_updated{0}
{
}

inline const Mouse& Context::mouse() const & {
  return m_mouse;
}

inline const Keyboard& Context::keyboard() const & {
  return m_keyboard;
}

inline const Vector<Controller>& Context::controllers() const & {
  return m_controllers;
}

inline const String& Context::clipboard() const & {
  return m_clipboard;
}

inline bool Context::is_mouse_captured() const {
  return m_mouse_captured;
}

inline void Context::capture_mouse(bool _capture) {
  if (_capture != m_mouse_captured) {
    m_mouse_captured = _capture;
    m_updated |= k_mouse_capture;
  }
}

inline void Context::capture_text(Text* text_) {
  if (m_active_text) {
    m_active_text->m_flags &= ~Text::k_active;
  }

  if (text_) {
    text_->m_flags |= Text::k_active;
    text_->clear();
  }

  m_active_text = text_;
}

inline Text* Context::active_text() const {
  return m_active_text;
}

inline void Context::change_clipboard_text(String&& text_) {
  m_clipboard = Utility::move(text_);
  m_updated |= k_clipboard;
}

} // namespace rx::input

#endif
