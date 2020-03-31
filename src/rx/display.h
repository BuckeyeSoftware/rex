#ifndef RX_DISPLAY_H
#define RX_DISPLAY_H
#include "rx/core/vector.h"
#include "rx/core/string.h"

#include "rx/core/hints/empty_bases.h"

#include "rx/math/vec2.h"
#include "rx/math/rectangle.h"

namespace rx {

struct RX_HINT_EMPTY_BASES display
  : concepts::no_copy
{
  display(memory::allocator& _allocator);
  display(display&& display_);

  struct mode {
    math::vec2z resolution;
    rx_f32 refresh_rate;
  };

  using extents = math::rectangle<rx_s32>;

  static vector<display> displays(memory::allocator& _allocator);

  // The display modes are sorted by the given priority:
  //  mode::resolution.w => largest to smallest
  //  mode::resolution.h => largest to smallest
  //  mode::refresh_rate => highest to lowest
  const vector<mode>& modes() const &;
  const string& name() const &;
  const extents& bounds() const &;

  rx_f32 diagonal_dpi() const;
  rx_f32 horizontal_dpi() const;
  rx_f32 vertical_dpi() const;

  // Check if the given extents are inside the display.
  bool contains(const extents& _extents) const;

  constexpr memory::allocator& allocator() const;

private:
  ref<memory::allocator> m_allocator;
  vector<mode> m_modes;
  string m_name;
  extents m_bounds;
  rx_f32 m_diagonal_dpi;
  rx_f32 m_horizontal_dpi;
  rx_f32 m_vertical_dpi;
};

inline display::display(memory::allocator& _allocator)
  : m_allocator{_allocator}
  , m_modes{allocator()}
  , m_name{allocator()}
  , m_diagonal_dpi{0.0f}
  , m_horizontal_dpi{0.0f}
  , m_vertical_dpi{0.0f}
{
}

inline display::display(display&& display_)
  : m_allocator{display_.m_allocator}
  , m_modes{utility::move(display_.m_modes)}
  , m_name{utility::move(display_.m_name)}
  , m_bounds{display_.m_bounds}
  , m_diagonal_dpi{display_.m_diagonal_dpi}
  , m_horizontal_dpi{display_.m_horizontal_dpi}
  , m_vertical_dpi{display_.m_vertical_dpi}
{
  display_.m_diagonal_dpi = 0.0f;
  display_.m_horizontal_dpi = 0.0f;
  display_.m_vertical_dpi = 0.0f;
  display_.m_bounds = {};
}

inline const vector<display::mode>& display::modes() const & {
  return m_modes;
}

inline const string& display::name() const & {
  return m_name;
}

inline const display::extents& display::bounds() const & {
  return m_bounds;
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

inline bool display::contains(const extents& _extents) const {
  return m_bounds.contains(_extents);
}

RX_HINT_FORCE_INLINE constexpr memory::allocator& display::allocator() const {
  return m_allocator;
}

} // namespace rx

#endif // RX_DISPLAY_H
