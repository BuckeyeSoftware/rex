#ifndef RX_INPUT_INPUT_H
#define RX_INPUT_INPUT_H

#include <rx/input/mouse.h>
#include <rx/input/keyboard.h>
#include <rx/input/event.h>

namespace rx::input {

struct input {
  void handle_event(event&& _event);
  void update(rx_f32 delta_time);

  const mouse& get_mouse() const { return m_mouse; }
  const keyboard& get_keyboard() const { return m_keyboard; }

private:
  mouse m_mouse;
  keyboard m_keyboard;
};

} // namespace rx::input

#endif
