#include <string.h> // memcpy

#include "rx/texture/chain.h"
#include "rx/texture/scale.h"
#include "rx/texture/convert.h"

#include "rx/core/math/log2.h"

#include "rx/core/utility/swap.h"

namespace rx::texture {

void chain::generate(vector<rx_byte>&& data_, pixel_format _has_format,
  pixel_format _want_format, const math::vec2z& _dimensions, bool _has_mipchain,
  bool _want_mipchain)
{
  if (_has_format == _want_format) {
    m_data = utility::move(data_);
  } else {
    m_data = convert(m_allocator, data_.data(), _dimensions.area(),
      _has_format, _want_format);
  }
  m_dimensions = _dimensions;
  m_pixel_format = _want_format;
  generate_mipchain(_has_mipchain, _want_mipchain);
}

void chain::generate(const rx_byte* _data, pixel_format _has_format,
  pixel_format _want_format, const math::vec2z& _dimensions, bool _has_mipchain,
  bool _want_mipchain)
{
  m_pixel_format = _want_format;
  m_dimensions = _dimensions;
  m_data.resize(_dimensions.area() * bpp(), utility::uninitialized{});
  if (_has_format == _want_format) {
    memcpy(m_data.data(), _data, m_data.size());
  } else {
    m_data = convert(m_allocator, _data, _dimensions.area(),
      _has_format, _want_format);
  }
  generate_mipchain(_has_mipchain, _want_mipchain);
}

static vector<chain::level> generate_levels(memory::allocator* _allocator,
  bool _want_mipchain, const math::vec2z& _dimensions, rx_size _bpp)
{
  vector<chain::level> result{_allocator};

  if (_want_mipchain) {
    // levels = log2(max(w, h)+1)
    const rx_size levels{math::log2(_dimensions.max_element()) + 1};
    result.reserve(levels);

    // calculate each miplevel in the chain
    math::vec2z dimensions{_dimensions};
    rx_size offset{0};
    for (rx_size i{0}; i < levels; i++) {
      const rx_size size{dimensions.area() * _bpp};
      result.push_back({offset, size, dimensions});
      offset += size;
      dimensions = dimensions.map([](rx_size _dim) {
        return algorithm::max(_dim / 2, 1_z);
      });
    };
  } else {
    result.push_back({0, _dimensions.area() * _bpp, _dimensions});
  }

  return result;
}

void chain::generate_mipchain(bool _has_mipchain, bool _want_mipchain) {
  m_levels = generate_levels(m_levels.allocator(), _want_mipchain,
    m_dimensions, bpp());

  // we have a mipchain but we don't want it
  if (_has_mipchain && !_want_mipchain) {
    // resize data for only base level
    m_data.resize(m_dimensions.area() * bpp(), utility::uninitialized{});
  }

  // we don't have a mipchain but we want one
  if (!_has_mipchain && _want_mipchain) {
    // calculate bytes needed for mipchain
    rx_size bytes_needed{0};
    m_levels.each_fwd([&bytes_needed](const level& _level) {
      bytes_needed += _level.size;
    });

    m_data.resize(bytes_needed, utility::uninitialized{});

    // use the previous miplevel in the mipchain to downsample for the current
    // level, that is, NxN is always generated from N*2xN*x
    for (rx_size i = 1; i < m_levels.size(); i++) {
      const auto& src_level{m_levels[i-1]};
      const auto& dst_level{m_levels[i]};

      const rx_byte* const src_data{m_data.data() + src_level.offset};
      rx_byte* const dst_data{m_data.data() + dst_level.offset};

      scale(src_data, src_level.dimensions.w, src_level.dimensions.h,
        bpp(), src_level.dimensions.w * bpp(), dst_data, dst_level.dimensions.w,
        dst_level.dimensions.h);
    }
  }
}

void chain::resize(const math::vec2z& _dimensions) {
  if (m_dimensions == _dimensions) {
    return;
  }

  // do this early because generate_mipchain uses this
  m_dimensions = _dimensions;

  // when we have a mipchain
  if (m_levels.size() > 1) {
    // find texture in mipchain closest to |_dimensions|
    rx_size best_index{0};
    for (rx_size i = 0; i < m_levels.size(); i++) {
      const auto& level{m_levels[i]};
      if (level.dimensions.w >= _dimensions.w && level.dimensions.h >= _dimensions.h) {
        best_index = i;
      }
    }

    const auto& level{m_levels[best_index]};

    // the mipchain image |best_index| is the appropriate size, just shift the
    // data up so that |best_index| is level 0
    if (level.dimensions == _dimensions) {
      // calculate the shrinked size for the mipchain
      rx_size new_size{0};
      for (rx_size i = best_index; i < m_levels.size(); i++) {
        new_size += m_levels[i].size;
      }

      // shrink to remove unnecessary data
      m_data.resize(new_size, utility::uninitialized{});

      // copy the mipchain from |best_index| forward to the beginning, this
      // is inplace and may overlap so memmove must be used
      memmove(m_data.data(), m_data.data() + level.offset, new_size);

      generate_mipchain(true, true);
    } else {
      // resize mipchain image |best_index| to |_dimensions|
      vector<rx_byte> data{m_data.allocator(), _dimensions.area() * bpp()};

      scale(m_data.data() + level.offset, level.dimensions.w,
        level.dimensions.h, bpp(), level.dimensions.w * bpp(), data.data(),
        _dimensions.w, _dimensions.h);

      // replace the data with the resized |best_index| level
      m_data = utility::move(data);

      // generate new mipchain
      generate_mipchain(false /*_has_mipchain*/, true /*_want_mipchain*/);
    }
  } else {
    // no mipchain

    // resize and move data
    vector<rx_byte> data{m_data.allocator(), _dimensions.area() * bpp()};
    scale(m_data.data(), m_dimensions.w, m_dimensions.h, bpp(),
      m_dimensions.w * bpp(), data.data(), _dimensions.w, _dimensions.h);

    m_data = utility::move(data);

    generate_mipchain(false /*_has_mipchain*/, false /*_want_mipchain*/);
  }
}

} // namespace rx::texture
