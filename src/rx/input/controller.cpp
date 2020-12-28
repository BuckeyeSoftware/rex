#include "rx/input/controller.h"
#include "rx/core/utility/exchange.h"

namespace Rx::Input {

Controller::Controller() {
  for (Size i = 0; i < BUTTONS; i++) {
    m_buttons[i] = 0;
  }

  for (Size i = 0; i < AXII; i++) {
    m_axis_values[i] = 0.0f;
  }
}

void Controller::update_button(bool _down, Button _button) {
  const auto index = static_cast<Size>(_button);

  if (_down) {
    m_buttons[index] |= (PRESSED | HELD);
  } else {
    m_buttons[index] |= RELEASED;
    m_buttons[index] &= ~HELD;
  }
}

void Controller::update_axis(Axis _axis, Float32 _value) {
  const auto index = static_cast<Size>(_axis);
  m_axis_values[index] = _value;
}

void Controller::update(Float32) {
  for (Size i = 0; i < BUTTONS; i++) {
    m_buttons[i] &= ~(PRESSED | RELEASED);
  }
}

} // namespace Rx::Input
