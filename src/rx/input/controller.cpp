#include <string.h> // memset

#include "rx/input/controller.h"

namespace Rx::Input {

Controller::Controller() {
  memset(m_buttons, 0, sizeof m_buttons);
  memset(m_axis_values, 0, sizeof m_axis_values);
}

void Controller::update_button(bool _down, Button _button) {
  const auto index = static_cast<Size>(_button);

  if (_down) {
    m_buttons[index] |= (k_pressed | k_held);
  } else {
    m_buttons[index] |= k_released;
    m_buttons[index] &= ~k_held;
  }
}

void Controller::update_axis(Axis _axis, Float32 _value) {
  const auto index = static_cast<Size>(_axis);
  m_axis_values[index] = _value;
}

void Controller::update(Float32) {
  for (Size i = 0; i < k_buttons; i++) {
    m_buttons[i] &= ~(k_pressed | k_released);
  }
}

} // namespace rx::input
