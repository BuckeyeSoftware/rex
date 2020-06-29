#ifndef RX_INPUT_CONTROLLER_H
#define RX_INPUT_CONTROLLER_H
#include "rx/core/types.h" // Size, Float32

namespace Rx::Input {

struct Controller {
  Controller();

  static constexpr const Size k_buttons{15};
  static constexpr const Size k_axii{6};

  enum class Button {
    k_a,
    k_b,
    k_x,
    k_y,
    k_back,
    k_guide,
    k_start,
    k_left_stick,
    k_right_stick,
    k_left_shoulder,
    k_right_shoulder,
    k_dpad_up,
    k_dpad_down,
    k_dpad_left,
    k_dpad_right
  };

  enum class Axis {
    k_left_x,       // in range [-1, 1]
    k_left_y,       // in range [-1, 1]
    k_right_x,      // in range [-1, 1]
    k_right_y,      // in range [-1, 1]
    k_trigger_left, // in range [0, 1]
    k_trigger_right // in range [0, 1]
  };

  void update_button(bool _down, Button _button);
  void update_axis(Axis _axis, Float32 _value);
  void update(Float32 _delta_time);

  bool is_pressed(Button _button) const;
  bool is_released(Button _button) const;
  bool is_held(Button _button) const;

  Float32 axis_value(Axis _axis) const;

private:
  enum {
    k_pressed  = 1 << 0,
    k_released = 1 << 1,
    k_held     = 1 << 2
  };

  int m_buttons[k_buttons];
  Float32 m_axis_values[k_axii];
};

inline bool Controller::is_pressed(Button _button) const {
  return m_buttons[static_cast<Size>(_button)] & k_pressed;
}

inline bool Controller::is_released(Button _button) const {
  return m_buttons[static_cast<Size>(_button)] & k_released;
}

inline bool Controller::is_held(Button _button) const {
  return m_buttons[static_cast<Size>(_button)] & k_held;
}

inline Float32 Controller::axis_value(Axis _axis) const {
  return m_axis_values[static_cast<Size>(_axis)];
}

} // namespace rx::input

#endif // RX_INPUT_CONTROLLER_H
