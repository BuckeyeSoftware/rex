#ifndef RX_INPUT_INPUT_H
#define RX_INPUT_INPUT_H

#include "rx/input/mouse.h"
#include "rx/input/keyboard.h"
#include "rx/input/event.h"

namespace rx::input {

struct input {
  void handle_event(event&& _event);
  void update(rx_f32 delta_time);

  const mouse_device& mouse() const;
  const keyboard_device& keyboard() const;

private:
  mouse_device m_mouse;
  keyboard_device m_keyboard;
};

inline const mouse_device& input::mouse() const {
  return m_mouse;
}

inline const keyboard_device& input::keyboard() const {
  return m_keyboard;
}

} // namespace rx::input

#endif
