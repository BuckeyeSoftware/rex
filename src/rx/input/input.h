#ifndef RX_INPUT_INPUT_H
#define RX_INPUT_INPUT_H
#include "rx/core/vector.h"

#include "rx/input/mouse.h"
#include "rx/input/keyboard.h"
#include "rx/input/event.h"
#include "rx/input/controller.h"

namespace rx::input {

struct input {
  input();
  input(memory::allocator* _allocator);

  void handle_event(const event& _event);
  void update(rx_f32 _delta_time);

  const mouse_device& mouse() const;
  const keyboard_device& keyboard() const;
  const vector<controller_device>& controllers() const;

private:
  mouse_device m_mouse;
  keyboard_device m_keyboard;
  vector<controller_device> m_controllers;
};

inline input::input()
  : input{&memory::g_system_allocator}
{
}

inline input::input(memory::allocator* _allocator)
  : m_controllers{_allocator}
{
}

inline const mouse_device& input::mouse() const {
  return m_mouse;
}

inline const keyboard_device& input::keyboard() const {
  return m_keyboard;
}

inline const vector<controller_device>& input::controllers() const {
  return m_controllers;
}

} // namespace rx::input

#endif
