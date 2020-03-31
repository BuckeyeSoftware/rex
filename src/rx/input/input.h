#ifndef RX_INPUT_INPUT_H
#define RX_INPUT_INPUT_H
#include "rx/core/vector.h"

#include "rx/input/mouse.h"
#include "rx/input/keyboard.h"
#include "rx/input/event.h"
#include "rx/input/controller.h"
#include "rx/input/text.h"

namespace rx::input {

struct input {
  enum updated {
    k_clipboard     = 1 << 0,
    k_mouse_capture = 1 << 1
  };

  input();
  input(memory::allocator* _allocator);

  void handle_event(const event& _event);
  int update(rx_f32 _delta_time);

  const mouse_device& mouse() const;
  const keyboard_device& keyboard() const;
  const vector<controller_device>& controllers() const;
  const string& clipboard() const;

  text* active_text() const;

  bool is_mouse_captured() const;

  void capture_mouse(bool _capture);
  void capture_text(text* _text);

private:
  void change_clipboard_text(string&& text_);

  mouse_device m_mouse;
  keyboard_device m_keyboard;
  vector<controller_device> m_controllers;
  text* m_active_text;
  string m_clipboard;
  bool m_mouse_captured;
  int m_updated;
};

inline input::input()
  : input{memory::system_allocator::instance()}
{
}

inline input::input(memory::allocator* _allocator)
  : m_controllers{_allocator}
  , m_active_text{nullptr}
  , m_clipboard{_allocator}
  , m_mouse_captured{true}
  , m_updated{0}
{
}

inline const mouse_device& input::mouse() const {
  return m_mouse;
}

inline const keyboard_device& input::keyboard() const {
  return m_keyboard;
}

inline const vector<controller_device>& input::controllers() const {
  return m_controllers;
}

inline const string& input::clipboard() const {
  return m_clipboard;
}

inline bool input::is_mouse_captured() const {
  return m_mouse_captured;
}

inline void input::capture_mouse(bool _capture) {
  if (_capture != m_mouse_captured) {
    m_mouse_captured = _capture;
    m_updated |= k_mouse_capture;
  }
}

inline void input::capture_text(text* _text) {
  if (m_active_text) {
    m_active_text->m_flags &= ~text::k_active;
  }

  if (_text) {
    _text->m_flags |= text::k_active;
    _text->clear();
  }

  m_active_text = _text;
}

inline text* input::active_text() const {
  return m_active_text;
}

inline void input::change_clipboard_text(string&& text_) {
  m_clipboard = utility::move(text_);
  m_updated |= k_clipboard;
}

} // namespace rx::input

#endif
