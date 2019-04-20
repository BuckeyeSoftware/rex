#ifndef RX_RENDER_TEXTURE_H
#define RX_RENDER_TEXTURE_H

#include <rx/render/resource.h>

#include <rx/core/array.h>
#include <rx/core/algorithm/max.h>

#include <rx/math/vec2.h>
#include <rx/math/vec3.h>
#include <rx/math/vec4.h>
#include <rx/math/log2.h>

namespace rx::render {

struct frontend;

struct texture : resource {
  texture(frontend* _frontend, resource::type _type);

  template<typename T>
  struct level_info {
    rx_size offset;
    rx_size size;
    T dimensions;
  };

  struct filter_options {
    bool bilinear;
    bool trilinear;
    bool mip_maps;
  };

  struct wrap_options {
    enum class type {
      k_clamp_to_edge,
      k_clamp_to_border,
      k_mirrored_repeat,
      k_repeat
    };

    type s;
    type t;
    type r;
  };

  enum class data_format {
    k_r_u8,
    k_rgba_u8,
    k_bgra_u8,
    k_rgba_f16,
    k_bgra_f16,
    k_d16,
    k_d24,
    k_d32,
    k_d32f,
    k_d24_s8,
    k_d32f_s8,
    k_s8
  };

  enum class type {
    k_attachment,
    k_static,
    k_dynamic
  };

  // get byte size of |_format|
  static rx_size byte_size_of_format(data_format _format);

  // get channel count of |_format|
  static rx_size channel_count_of_format(data_format _format);

  // record format |_format|
  void record_format(data_format _format);

  // record type |_type|
  void record_type(type _type);

  // record |_wrap| parameters
  void record_wrap(const wrap_options& _options);

  // record |_filter| parameters
  void record_filter(const filter_options& _options);

  void validate() const;

  const array<rx_byte>& data() const &;

  data_format format() const;
  filter_options filter() const;
  wrap_options wrap() const;
  rx_size channels() const;
  type kind() const;

protected:
  enum : rx_u16 {
    k_format     = 1 << 0,
    k_type       = 1 << 1,
    k_filter     = 1 << 2,
    k_wrap       = 1 << 3,
    k_dimensions = 1 << 4
  };

  array<rx_byte> m_data;
  data_format m_format;
  type m_type;
  filter_options m_filter;
  wrap_options m_wrap;
  rx_u16 m_recorded;
};

struct texture1D : texture {
  texture1D(frontend* _frontend);
  ~texture1D();

  // write data |_data| to store for miplevel |_level|
  void write(const rx_byte* _data, rx_size _level);

  // record dimensions |_dimensions|
  void record_dimensions(rx_size _dimensions);

  rx_size dimensions() const;
  rx_size levels() const;

  level_info<rx_size> info_for_level(rx_size _level) const;

private:
  rx_size m_dimensions;
  rx_size m_dimensions_log2;
};

struct texture2D : texture {
  texture2D(frontend* _frontend);
  ~texture2D();

  // write data |_data| to store for miplevel |_level|
  void write(const rx_byte* _data, rx_size _level);
  void record_dimensions(const math::vec2z& _dimensions);

  const math::vec2z& dimensions() const &;
  rx_size levels() const;

  level_info<math::vec2z> info_for_level(rx_size _level) const;

private:
  math::vec2z m_dimensions;
  math::vec2z m_dimensions_log2;
};

struct texture3D : texture {
  texture3D(frontend* _frontend);
  ~texture3D();

  // write 3D data |_data| to store for miplevel |_level|
  void write(const rx_byte* _data, rx_size _level);
  void record_dimensions(const math::vec3z& _dimensions);

  const math::vec3z& dimensions() const &;
  rx_size levels() const;

  level_info<math::vec3z> info_for_level(rx_size _level) const;

private:
  math::vec3z m_dimensions;
  math::vec3z m_dimensions_log2;
};

struct textureCM : texture {
  enum class face : rx_u8 {
    k_right,
    k_left,
    k_top,
    k_bottom,
    k_front,
    k_back
  };

  textureCM(frontend* _frontend);
  ~textureCM();

  // write data |_data| for face |_face| to store for miplevel |_level|
  void write(const rx_byte* _data, face _face, rx_size _level);
  void record_dimensions(const math::vec2z& _dimensions);

  const math::vec2z& dimensions() const &;
  rx_size levels() const;

  level_info<math::vec2z> info_for_level(face _face, rx_size _level) const;

private:
  math::vec2z m_dimensions;
  math::vec2z m_dimensions_log2;
};

inline rx_size texture1D::dimensions() const {
  return m_dimensions;
}

inline rx_size texture1D::levels() const {
  return m_filter.mip_maps ? math::log2(m_dimensions_log2) + 1 : 1;
}

inline const math::vec2z& texture2D::dimensions() const & {
  return m_dimensions;
}

inline rx_size texture2D::levels() const {
  return m_filter.mip_maps ? math::log2(algorithm::max(m_dimensions.w, m_dimensions.h)) + 1 : 1;
}

inline const math::vec3z& texture3D::dimensions() const & {
  return m_dimensions;
}

inline rx_size texture3D::levels() const {
  return m_filter.mip_maps ? math::log2(algorithm::max(m_dimensions.w, m_dimensions.h, m_dimensions.d)) + 1 : 1;
}

inline const math::vec2z& textureCM::dimensions() const & {
  return m_dimensions;
}

inline rx_size textureCM::levels() const {
  return m_filter.mip_maps ? math::log2(algorithm::max(m_dimensions.w, m_dimensions.h)) + 1 : 1;
}

// texture
inline const array<rx_byte>& texture::data() const & {
  return m_data;
}

inline texture::data_format texture::format() const {
  return m_format;
}

inline texture::filter_options texture::filter() const {
  return m_filter;
}

inline rx_size texture::channels() const {
  return channel_count_of_format(m_format);
}

inline texture::type texture::kind() const {
  return m_type;
}

inline texture::wrap_options texture::wrap() const {
  return m_wrap;
}

inline rx_size texture::byte_size_of_format(data_format _format) {
  switch (_format) {
  case data_format::k_rgba_u8:
    return 4;
  case data_format::k_bgra_u8:
    return 4;
  case data_format::k_rgba_f16:
    return 8;
  case data_format::k_bgra_f16:
    return 8;
  case data_format::k_d16:
    return 2;
  case data_format::k_d24:
    return 3;
  case data_format::k_d32:
    return 4;
  case data_format::k_d32f:
    return 4;
  case data_format::k_d24_s8:
    return 4;
  case data_format::k_d32f_s8:
    return 5;
  case data_format::k_s8:
    return 1;
  case data_format::k_r_u8:
    return 1;
  }
  return 0;
}

inline rx_size texture::channel_count_of_format(data_format _format) {
  switch (_format) {
  case data_format::k_rgba_u8:
    return 4;
  case data_format::k_bgra_u8:
    return 4;
  case data_format::k_rgba_f16:
    return 4;
  case data_format::k_bgra_f16:
    return 4;
  case data_format::k_d16:
    return 1;
  case data_format::k_d24:
    return 1;
  case data_format::k_d32:
    return 1;
  case data_format::k_d32f:
    return 1;
  case data_format::k_d24_s8:
    return 2;
  case data_format::k_d32f_s8:
    return 2;
  case data_format::k_s8:
    return 1;
  case data_format::k_r_u8:
    return 1;
  }
  return 0;
}

} // namespace rx::render

#endif // RX_RENDER_TEXTURE_H
