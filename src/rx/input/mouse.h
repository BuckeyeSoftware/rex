#ifndef RX_INPUT_MOUSE_H
#define RX_INPUT_MOUSE_H

#include "rx/math/vec2.h"
#include "rx/math/vec4.h"

namespace Rx::Input {

struct Mouse {
  Mouse();

  static constexpr const auto k_buttons{32};

  void update(Float32 _delta_time);
  void update_button(bool _down, int _button);
  void update_motion(const Math::Vec4i& _movement);
  void update_scroll(const Math::Vec2i& _scroll);

  const Math::Vec2i& movement() const;
  const Math::Vec2i& position() const;
  const Math::Vec2i& scroll() const;

  bool is_pressed(int _button) const;
  bool is_held(int _button) const;
  bool is_released(int _button) const;

private:
  enum {
    k_pressed  = 1 << 0,
    k_released = 1 << 1,
    k_held     = 1 << 2
  };

  int m_buttons[k_buttons];

  Math::Vec2i m_position;
  Math::Vec2i m_movement;
  Math::Vec2i m_scroll;

  bool m_scrolled;
};

inline const Math::Vec2i& Mouse::movement() const {
  return m_movement;
}

inline const Math::Vec2i& Mouse::position() const {
  return m_position;
}

inline const Math::Vec2i& Mouse::scroll() const {
  return m_scroll;
}

inline bool Mouse::is_pressed(int _button) const {
  RX_ASSERT(_button && _button < k_buttons, "out of range");
  return m_buttons[_button] & k_pressed;
}

inline bool Mouse::is_held(int _button) const {
  RX_ASSERT(_button && _button < k_buttons, "out of range");
  return m_buttons[_button] & k_held;
}

inline bool Mouse::is_released(int _button) const {
  RX_ASSERT(_button && _button < k_buttons, "out of range");
  return m_buttons[_button] & k_released;
}

} // namspace rx::input

#endif // RX_INPUT_MOUSE_H
