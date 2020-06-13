#ifndef RX_DISPLAY_H
#define RX_DISPLAY_H
#include "rx/core/vector.h"
#include "rx/core/string.h"

#include "rx/core/hints/empty_bases.h"

#include "rx/math/vec2.h"
#include "rx/math/rectangle.h"

namespace Rx {

struct RX_HINT_EMPTY_BASES Display
  : Concepts::NoCopy
{
  Display(Memory::Allocator& _allocator);
  Display(Display&& display_);

  struct Mode {
    Math::Vec2z resolution;
    Float32 refresh_rate;
  };

  using Extents = Math::rectangle<Sint32>;

  static Vector<Display> displays(Memory::Allocator& _allocator);

  // The display modes are sorted by the given priority:
  //  mode::resolution.w => largest to smallest
  //  mode::resolution.h => largest to smallest
  //  mode::refresh_rate => highest to lowest
  const Vector<Mode>& modes() const &;
  const String& name() const &;
  const Extents& bounds() const &;

  Float32 diagonal_dpi() const;
  Float32 horizontal_dpi() const;
  Float32 vertical_dpi() const;

  // Check if the given extents are inside the display.
  bool contains(const Extents& _extents) const;

  constexpr Memory::Allocator& allocator() const;

private:
  Ref<Memory::Allocator> m_allocator;
  Vector<Mode> m_modes;
  String m_name;
  Extents m_bounds;
  Float32 m_diagonal_dpi;
  Float32 m_horizontal_dpi;
  Float32 m_vertical_dpi;
};

inline Display::Display(Memory::Allocator& _allocator)
  : m_allocator{_allocator}
  , m_modes{allocator()}
  , m_name{allocator()}
  , m_diagonal_dpi{0.0f}
  , m_horizontal_dpi{0.0f}
  , m_vertical_dpi{0.0f}
{
}

inline Display::Display(Display&& display_)
  : m_allocator{display_.m_allocator}
  , m_modes{Utility::move(display_.m_modes)}
  , m_name{Utility::move(display_.m_name)}
  , m_bounds{display_.m_bounds}
  , m_diagonal_dpi{Utility::exchange(display_.m_diagonal_dpi, 0.0f)}
  , m_horizontal_dpi{Utility::exchange(display_.m_horizontal_dpi, 0.0f)}
  , m_vertical_dpi{Utility::exchange(display_.m_vertical_dpi, 0.0f)}
{
  display_.m_bounds = {};
}

inline const Vector<Display::Mode>& Display::modes() const & {
  return m_modes;
}

inline const String& Display::name() const & {
  return m_name;
}

inline const Display::Extents& Display::bounds() const & {
  return m_bounds;
}

inline Float32 Display::diagonal_dpi() const {
  return m_diagonal_dpi;
}

inline Float32 Display::horizontal_dpi() const {
  return m_horizontal_dpi;
}

inline Float32 Display::vertical_dpi() const {
  return m_vertical_dpi;
}

inline bool Display::contains(const Extents& _extents) const {
  return m_bounds.contains(_extents);
}

RX_HINT_FORCE_INLINE constexpr Memory::Allocator& Display::allocator() const {
  return m_allocator;
}

} // namespace rx

#endif // RX_DISPLAY_H
