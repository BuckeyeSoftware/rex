#include "rx/texture/chain.h"
#include "rx/texture/scale.h"
#include "rx/texture/convert.h"

#include "rx/core/math/log2.h"

#include "rx/core/utility/swap.h"

#include "rx/core/memory/move.h"

namespace Rx::Texture {

bool Chain::generate(LinearBuffer&& data_, PixelFormat _has_format,
  PixelFormat _want_format, const Math::Vec2z& _dimensions, bool _has_mipchain,
  bool _want_mipchain)
{
  if (_has_format == _want_format) {
    m_data = Utility::move(data_);
  } else if (auto data = convert(allocator(), data_.data(), _dimensions.area(), _has_format, _want_format)) {
    m_data = Utility::move(*data);
  } else {
    return false;
  }

  m_dimensions = _dimensions;
  m_pixel_format = _want_format;

  return generate_mipchain(_has_mipchain, _want_mipchain);
}

bool Chain::generate(const Byte* _data, PixelFormat _has_format,
  PixelFormat _want_format, const Math::Vec2z& _dimensions, bool _has_mipchain,
  bool _want_mipchain)
{
  m_pixel_format = _want_format;
  m_dimensions = _dimensions;

  if (!m_data.resize(_dimensions.area() * bpp())) {
    return false;
  }

  if (_has_format == _want_format) {
    Memory::copy(m_data.data(), _data, m_data.size());
  } else if (auto data = convert(allocator(), _data, _dimensions.area(), _has_format, _want_format)) {
    m_data = Utility::move(*data);
  } else {
    return false;
  }

  return generate_mipchain(_has_mipchain, _want_mipchain);
}

static Optional<Vector<Chain::Level>>
generate_levels(Memory::Allocator& _allocator, bool _want_mipchain,
  const Math::Vec2z& _dimensions, Size _bpp)
{
  Vector<Chain::Level> result{_allocator};

  if (_want_mipchain) {
    // levels = log2(max(w, h)+1)
    const auto levels =
      Math::log2(Uint64(_dimensions.max_element())) + 1;
    if (!result.reserve(levels)) {
      return nullopt;
    }

    // calculate each miplevel in the chain
    Math::Vec2z dimensions{_dimensions};
    Size offset = 0;
    for (Size i = 0; i < levels; i++) {
      const auto size = dimensions.area() * _bpp;
      if (!result.emplace_back(offset, size, dimensions)) {
        return nullopt;
      }
      offset += size;
      dimensions = dimensions.map([](Size _dim) {
        return Algorithm::max(_dim / 2, 1_z);
      });
    };
  } else {
    if (!result.emplace_back(0_z, _dimensions.area() * _bpp, _dimensions)) {
      return nullopt;
    }
  }

  return result;
}

bool Chain::generate_mipchain(bool _has_mipchain, bool _want_mipchain) {
  auto levels = generate_levels(m_levels.allocator(), _want_mipchain,
    m_dimensions, bpp());
  if (!levels) {
    return false;
  }

  m_levels = Utility::move(*levels);

  // we have a mipchain but we don't want it
  if (_has_mipchain && !_want_mipchain) {
    // Resize data for only base level.
    if (!m_data.resize(m_dimensions.area() * bpp())) {
      return false;
    }
  }

  // We don't have a mipchain but we want one
  if (!_has_mipchain && _want_mipchain) {
    // Calculate bytes needed for mipchain
    Size bytes_needed = 0;
    m_levels.each_fwd([&bytes_needed](const Level& _level) {
      bytes_needed += _level.size;
    });

    // Resize for mipchain.
    if (!m_data.resize(bytes_needed)) {
      return false;
    }

    // Use the previous miplevel in the mipchain to downsample for the current
    // level, that is, NxN is always generated from N*2xN*x
    for (Size i = 1; i < m_levels.size(); i++) {
      const auto& src_level = m_levels[i-1];
      const auto& dst_level = m_levels[i];

      const Byte* const src_data = m_data.data() + src_level.offset;
      Byte* const dst_data = m_data.data() + dst_level.offset;

      scale(src_data, src_level.dimensions.w, src_level.dimensions.h,
        bpp(), src_level.dimensions.w * bpp(), dst_data, dst_level.dimensions.w,
        dst_level.dimensions.h);
    }
  }

  return true;
}

bool Chain::resize(const Math::Vec2z& _dimensions) {
  // No need to resize when the dimensions are the same.
  if (m_dimensions == _dimensions) {
    return true;
  }

  // Do this early because |generate_mipchain| uses this.
  m_dimensions = _dimensions;

  // Only have the base level, nothing to do to resize.
  if (m_levels.size() == 1) {
    return true;
  }

  // Find texture in mipchain closest to |_dimensions|.
  Size best_index = 0;
  for (Size i = 0; i < m_levels.size(); i++) {
    const auto& level = m_levels[i];
    if (level.dimensions.w >= _dimensions.w && level.dimensions.h >= _dimensions.h) {
      best_index = i;
    }
  }

  const auto& level = m_levels[best_index];

  // The mipchain image |best_index| is the appropriate size, just shift the
  // data up so that |best_index| is at level 0.
  if (level.dimensions == _dimensions) {
    // Calculate the shrinked size for the mipchain.
    Size new_size = 0;
    for (Size i = best_index; i < m_levels.size(); i++) {
      new_size += m_levels[i].size;
    }

    // Shrink to remove unnecessary data. This cannot fail.
    (void)m_data.resize(new_size);

    // Copy the mipchain from |best_index| forward to the beginning, this
    // is inplace and may overlap so memmove must be used.
    Memory::move(m_data.data(), m_data.data() + level.offset, new_size);

    return generate_mipchain(true, true);
  } else {
    // Resize mipchain image |best_index| to |_dimensions|.
    LinearBuffer data{m_data.allocator()};
    if (!data.resize(_dimensions.area() * bpp())) {
      return false;
    }

    scale(m_data.data() + level.offset, level.dimensions.w,
      level.dimensions.h, bpp(), level.dimensions.w * bpp(), data.data(),
      _dimensions.w, _dimensions.h);

    // Replace the data with the resized |best_index| level.
    m_data = Utility::move(data);

    // Generate new mipchain
    return generate_mipchain(false /*_has_mipchain*/, true /*_want_mipchain*/);
  }

  // No mipchain.

  // Resize and move data.
  LinearBuffer data{m_data.allocator()};
  if (!data.resize(_dimensions.area() * bpp())) {
    return false;
  }

  scale(m_data.data(), m_dimensions.w, m_dimensions.h, bpp(),
    m_dimensions.w * bpp(), data.data(), _dimensions.w, _dimensions.h);

  m_data = Utility::move(data);

  return generate_mipchain(false /*_has_mipchain*/, false /*_want_mipchain*/);
}

} // namespace Rx::Texture
