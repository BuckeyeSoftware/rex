#include <string.h> // memset

#include "rx/input/controller.h"

namespace rx::input {

controller_device::controller_device()
{
  memset(m_buttons, 0, sizeof m_buttons);
  memset(m_axis_values, 0, sizeof m_axis_values);
}

void controller_device::update_button(bool _down, button _button) {
  const auto index{static_cast<rx_size>(_button)};

  if (_down) {
    m_buttons[index] |= (k_pressed | k_held);
  } else {
    m_buttons[index] |= k_released;
    m_buttons[index] &= ~k_held;
  }
}

void controller_device::update_axis(axis _axis, rx_f32 _value) {
  const auto index{static_cast<rx_size>(_axis)};
  m_axis_values[index] = _value;
}

void controller_device::update(rx_f32) {
  for (rx_size i{0}; i < k_buttons; i++) {
    m_buttons[i] &= ~(k_pressed | k_released);
  }
}

} // namespace rx::input