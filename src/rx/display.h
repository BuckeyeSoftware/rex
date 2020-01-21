#ifndef RX_DISPLAY_H
#define RX_DISPLAY_H
#include "rx/core/vector.h"
#include "rx/core/string.h"

#include "rx/math/vec2.h"

namespace rx {

struct display
  : concepts::no_copy
{
  display(memory::allocator* _allocator);
  display(display&& display_);

  struct mode {
    math::vec2z resolution;
    rx_f32 refresh_rate;
  };

  static vector<display> displays(memory::allocator* _allocator);

  // The display modes are sorted by the given priority:
  //  mode::resolution.w => largest to smallest
  //  mode::resolution.h => largest to smallest
  //  mode::refresh_rate => highest to lowest
  const vector<mode>& modes() const &;
  const string& name() const &;

  rx_f32 diagonal_dpi() const;
  rx_f32 horizontal_dpi() const;
  rx_f32 vertical_dpi() const;

private:
  vector<mode> m_modes;
  string m_name;
  rx_f32 m_diagonal_dpi;
  rx_f32 m_horizontal_dpi;
  rx_f32 m_vertical_dpi;
};

inline display::display(memory::allocator* _allocator)
  : m_modes{_allocator}
  , m_name{_allocator}
  , m_diagonal_dpi{0.0f}
  , m_horizontal_dpi{0.0f}
  , m_vertical_dpi{0.0f}
{
}

inline display::display(display&& display_)
  : m_modes{utility::move(display_.m_modes)}
  , m_name{utility::move(display_.m_name)}
  , m_diagonal_dpi{display_.m_diagonal_dpi}
  , m_horizontal_dpi{display_.m_horizontal_dpi}
  , m_vertical_dpi{display_.m_vertical_dpi}
{
  display_.m_diagonal_dpi = 0.0f;
  display_.m_horizontal_dpi = 0.0f;
  display_.m_vertical_dpi = 0.0f;
}

inline const vector<display::mode>& display::modes() const & {
  return m_modes;
}

inline const string& display::name() const & {
  return m_name;
}

inline rx_f32 display::diagonal_dpi() const {
  return m_diagonal_dpi;
}

inline rx_f32 display::horizontal_dpi() const {
  return m_horizontal_dpi;
}

inline rx_f32 display::vertical_dpi() const {
  return m_vertical_dpi;
}

} // namespace rx

#endif // RX_DISPLAY_H
