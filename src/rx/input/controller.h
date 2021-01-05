#ifndef RX_INPUT_CONTROLLER_H
#define RX_INPUT_CONTROLLER_H
#include "rx/core/types.h" // Size, Float32

namespace Rx::Input {

struct Controller {
  Controller();

  static inline constexpr const Size BUTTONS = 15;
  static inline constexpr const Size AXII = 6;

  enum class Button : Uint8 {
    A,
    B,
    X,
    Y,
    BACKK,
    GUIDE,
    START,
    LEFT_STICK,
    RIGHT_STICK,
    LEFT_SHOULDER,
    RIGHT_SHOULDER,
    DPAD_UP,
    DPAD_DOWN,
    DPAD_LEFT,
    DPAD_RIGHT
  };

  enum class Axis : Uint8 {
    LEFT_X,       // in range [-1, 1]
    LEFT_Y,       // in range [-1, 1]
    RIGHT_X,      // in range [-1, 1]
    RIGHT_Y,      // in range [-1, 1]
    TRIGGER_LEFT, // in range [0, 1]
    TRIGGER_RIGHT // in range [0, 1]
  };

  void update_button(bool _down, Button _button);
  void update_axis(Axis _axis, Float32 _value);
  void update(Float32 _delta_time);

  bool is_pressed(Button _button) const;
  bool is_released(Button _button) const;
  bool is_held(Button _button) const;

  Float32 axis_value(Axis _axis) const;

private:
  enum : Uint8 {
    PRESSED  = 1 << 0,
    RELEASED = 1 << 1,
    HELD     = 1 << 2
  };

  Uint8 m_buttons[BUTTONS];
  Float32 m_axis_values[AXII];
};

inline bool Controller::is_pressed(Button _button) const {
  return m_buttons[static_cast<Size>(_button)] & PRESSED;
}

inline bool Controller::is_released(Button _button) const {
  return m_buttons[static_cast<Size>(_button)] & RELEASED;
}

inline bool Controller::is_held(Button _button) const {
  return m_buttons[static_cast<Size>(_button)] & HELD;
}

inline Float32 Controller::axis_value(Axis _axis) const {
  return m_axis_values[static_cast<Size>(_axis)];
}

} // namespace Rx::Input

#endif // RX_INPUT_CONTROLLER_H
