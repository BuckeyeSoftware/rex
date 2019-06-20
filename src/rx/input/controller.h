#ifndef RX_INPUT_CONTROLLER_H
#define RX_INPUT_CONTROLLER_H

#include "rx/core/function.h"

namespace rx::input {

struct controller_device {
  controller_device();
  
  static constexpr const rx_size k_buttons{15};
  static constexpr const rx_size k_axii{6};

  enum class button {
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

  enum class axis {
    k_left_x,       // in range [-1, 1]
    k_left_y,       // in range [-1, 1]
    k_right_x,      // in range [-1, 1]
    k_right_y,      // in range [-1, 1]
    k_trigger_left, // in range [0, 1]
    k_trigger_right // in range [0, 1]
  };

  void update_button(bool _down, button _button);
  void update_axis(axis _axis, rx_f32 _value);
  void update(rx_f32 _delta_time);

  bool is_pressed(button _button) const;
  bool is_released(button _button) const;
  bool is_held(button _button) const;

  rx_f32 axis_value(axis _axis) const;

private:
  enum {
    k_pressed  = 1 << 0,
    k_released = 1 << 1,
    k_held     = 1 << 2
  };

  int m_buttons[k_buttons];
  rx_f32 m_axis_values[k_axii];
};

inline bool controller_device::is_pressed(button _button) const {
  return m_buttons[static_cast<rx_size>(_button)] & k_pressed;
}

inline bool controller_device::is_released(button _button) const {
  return m_buttons[static_cast<rx_size>(_button)] & k_released;
}

inline bool controller_device::is_held(button _button) const {
  return m_buttons[static_cast<rx_size>(_button)] & k_held;
}

inline rx_f32 controller_device::axis_value(axis _axis) const {
  return m_axis_values[static_cast<rx_size>(_axis)];
}

} // namespace rx::input

#endif // RX_INPUT_CONTROLLER_H