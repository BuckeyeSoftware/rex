#ifndef RX_INPUT_EVENT_H
#define RX_INPUT_EVENT_H
#include "rx/math/vec2.h"
#include "rx/math/vec4.h"

#include "rx/core/string.h"
#include "rx/core/utility/nat.h"

#include "rx/input/controller.h"

namespace Rx::Input {

struct KeyboardEvent {
  bool down;
  int scan_code;
  int symbol;
};

struct ControllerNotificationEvent {
  enum class Type : Uint8 {
    CONNECTED,
    DISCONNECTED
  };

  Size index;
  Type kind;
};

struct ControllerButtonEvent {
  Size index;
  bool down;
  Controller::Button button;
};

struct ControllerMotionEvent {
  Size index;
  Controller::Axis axis;
  Float32 value;
};

struct MouseButtonEvent {
  bool down;
  int button;
  Math::Vec2i position;
};

struct MouseScrollEvent {
  Math::Vec2i value;
};

struct MouseMotionEvent {
  Math::Vec4i value; // x, y, xrel, yrel
};

struct TextInputEvent {
  char contents[32];
};

struct ClipboardEvent {
  String contents;
};

struct Event {
  constexpr Event();
  ~Event();

  enum class Type : Uint8 {
    NONE,
    KEYBOARD,
    CONTROLLER_NOTIFICATION,
    CONTROLLER_BUTTON,
    CONTROLLER_MOTION,
    MOUSE_BUTTON,
    MOUSE_SCROLL,
    MOUSE_MOTION,
    TEXT_INPUT,
    CLIPBOARD
  };

  Type type;

  union {
    Utility::Nat as_nat;
    KeyboardEvent as_keyboard;
    ControllerNotificationEvent as_controller_notification;
    ControllerButtonEvent as_controller_button;
    ControllerMotionEvent as_controller_motion;
    MouseButtonEvent as_mouse_button;
    MouseScrollEvent as_mouse_scroll;
    MouseMotionEvent as_mouse_motion;
    TextInputEvent as_text_input;
    ClipboardEvent as_clipboard;
  };
};

inline constexpr Event::Event()
  : type{Type::NONE}
  , as_nat{}
{
}

inline Event::~Event() {
  if (type == Type::CLIPBOARD) {
    Utility::destruct<String>(&as_clipboard.contents);
  }
}

} // namespace Rx::Input

#endif // RX_INPUT_EVENT_H
