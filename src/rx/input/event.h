#ifndef RX_INPUT_EVENT_H
#define RX_INPUT_EVENT_H
#include "rx/math/vec2.h"
#include "rx/math/vec4.h"

#include "rx/core/string.h"
#include "rx/core/utility/nat.h"

#include "rx/input/controller.h"

namespace rx::input {

enum class event_type {
  k_none,
  k_keyboard,
  k_controller_notification,
  k_controller_button,
  k_controller_motion,
  k_mouse_button,
  k_mouse_scroll,
  k_mouse_motion,
  k_text_input,
  k_clipboard
};

struct keyboard_event {
  bool down;
  int scan_code;
  int symbol;
};

struct controller_notification_event {
  enum class type {
    k_connected,
    k_disconnected
  };

  rx_size index;
  type kind;
};

struct controller_button_event {
  rx_size index;
  bool down;
  controller_device::button button;
};

struct controller_motion_event {
  rx_size index;
  controller_device::axis axis;
  rx_f32 value;
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

struct text_input_event {
  char contents[32];
};

struct clipboard_event {
  string contents;
};

struct event {
  constexpr event();
  ~event();

  event_type type;

  union {
    utility::nat as_nat;
    keyboard_event as_keyboard;
    controller_notification_event as_controller_notification;
    controller_button_event as_controller_button;
    controller_motion_event as_controller_motion;
    mouse_button_event as_mouse_button;
    mouse_scroll_event as_mouse_scroll;
    mouse_motion_event as_mouse_motion;
    text_input_event as_text_input;
    clipboard_event as_clipboard;
  };
};

inline constexpr event::event()
  : type{event_type::k_none}
  , as_nat{}
{
}

inline event::~event() {
  if (type == event_type::k_clipboard) {
    as_clipboard.contents.~string();
  }
}

} // namespace rx::input

#endif // RX_INPUT_EVENT_H
