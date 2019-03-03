#ifndef RX_INPUT_EVENT_H
#define RX_INPUT_EVENT_H

#include <rx/math/vec2.h> // math::vec2i
#include <rx/core/traits.h> // nat

namespace rx::input {

enum class event_type {
  k_keyboard,
  k_mouse_button,
  k_mouse_scroll,
  k_mouse_motion
};

struct keyboard_event {
  bool down;
  int scan_code;
  int symbol;
};

struct mouse_button_event {
  bool down;
  int button;
};

struct mouse_scroll_event {
  math::vec2i value;
};

struct mouse_motion_event {
  math::vec2i value;
};

struct event {
  event() : as_nat{} {}

  event_type type;

  union {
    nat as_nat;
    keyboard_event as_keyboard;
    mouse_button_event as_mouse_button;
    mouse_scroll_event as_mouse_scroll;
    mouse_motion_event as_mouse_motion;
  };
};

} // namespace rx::input

#endif // RX_INPUT_EVENT_H
