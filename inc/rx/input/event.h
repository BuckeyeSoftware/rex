#ifndef RX_INPUT_EVENT_H
#define RX_INPUT_EVENT_H

#include <rx/math/vec2.h>
#include <rx/math/vec4.h>

#include <rx/core/utility/nat.h>

namespace rx::input {

enum class event_type {
  k_none,
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
  math::vec4i value; // x, y, xrel, yrel
};

struct event {
  constexpr event();

  event_type type;

  union {
    utility::nat as_nat;
    keyboard_event as_keyboard;
    mouse_button_event as_mouse_button;
    mouse_scroll_event as_mouse_scroll;
    mouse_motion_event as_mouse_motion;
  };
};

inline constexpr event::event()
  : type{event_type::k_none}
  , as_nat{}
{
}

} // namespace rx::input

#endif // RX_INPUT_EVENT_H
