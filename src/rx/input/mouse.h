#ifndef RX_INPUT_MOUSE_H
#define RX_INPUT_MOUSE_H

#include "rx/math/vec2.h"
#include "rx/math/vec4.h"

namespace rx::input {

struct mouse_device {
  mouse_device();

  static constexpr const auto k_buttons{32};

  void update(rx_f32 delta_time);
  void update_button(bool down, int button);
  void update_motion(const math::vec4i& movement);
  void update_scroll(const math::vec2i& scroll);

  const math::vec2i& movement() const;
  const math::vec2i& position() const;
  const math::vec2i& scroll() const;

private:
  enum {
    k_pressed = 1 << 0,
    k_released = 1 << 1,
    k_held = 1 << 2
  };

  int m_buttons[k_buttons];

  math::vec2i m_position;
  math::vec2i m_movement;
  math::vec2i m_scroll;

  bool m_scrolled;
};

inline const math::vec2i& mouse_device::movement() const {
  return m_movement;
}

inline const math::vec2i& mouse_device::position() const {
  return m_position;
}

inline const math::vec2i& mouse_device::scroll() const {
  return m_scroll;
}

} // namspace rx::input

#endif // RX_INPUT_MOUSE_H
