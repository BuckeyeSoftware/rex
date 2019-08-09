#ifndef RX_RENDER_FRONTEND_TEXTURE_H
#define RX_RENDER_FRONTEND_TEXTURE_H

#include "rx/render/frontend/resource.h"

#include "rx/core/array.h"
#include "rx/core/algorithm/max.h"
#include "rx/core/math/log2.h"

#include "rx/math/vec2.h"
#include "rx/math/vec3.h"
#include "rx/math/vec4.h"


namespace rx::render::frontend {

struct interface;

struct texture : resource {
  texture(interface* _frontend, resource::type _type);

  template<typename T>
  struct level_info {
    rx_size offset;
    rx_size size;
    T dimensions;
  };

  struct filter_options {
    bool bilinear;
    bool trilinear;
    bool mipmaps;
  };

  enum class wrap_type : rx_u8 {
    k_clamp_to_edge,
    k_clamp_to_border,
    k_mirrored_repeat,
    k_mirror_clamp_to_edge,
    k_repeat
  };

  enum class data_format : rx_u8 {
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
    k_s8,
    k_dxt1,
    k_dxt5
  };

  enum class type : rx_u8 {
    k_attachment,
    k_static,
    k_dynamic
  };

  // get byte size for one pixel of |_format|
  // NOTE: can be fractional for compressed block formats
  static rx_f32 byte_size_of_format(data_format _format);

  // get channel count of |_format|
  static rx_size channel_count_of_format(data_format _format);

  // record format |_format|
  void record_format(data_format _format);

  // record type |_type|
  void record_type(type _type);

  // record filter options |_options|
  void record_filter(const filter_options& _options);

  void validate() const;

  const array<rx_byte>& data() const &;
  data_format format() const;
  filter_options filter() const;
  rx_size channels() const;
  type kind() const;
  bool is_compressed() const;

protected:
  enum : rx_u8 {
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
  rx_u16 m_recorded;
};

struct texture1D : texture {
  using dimension_type = rx_size;
  using wrap_options = wrap_type;

  texture1D(interface* _frontend);
  ~texture1D();

  // write data |_data| to store for miplevel |_level|
  void write(const rx_byte* _data, rx_size _level);

  // map data for miplevel |_level|
  rx_byte* map(rx_size _level);

  void record_dimensions(const dimension_type& _dimensions);
  void record_wrap(const wrap_options& _wrap);

  const dimension_type& dimensions() const &;
  const wrap_options& wrap() const &;
  rx_size levels() const;
  const level_info<dimension_type>& info_for_level(rx_size _index) const &;

private:
  dimension_type m_dimensions;
  wrap_options m_wrap;
  array<level_info<dimension_type>> m_levels;
};

struct texture2D : texture {
  using dimension_type = math::vec2z;
  using wrap_options = math::vec2<wrap_type>;

  texture2D(interface* _frontend);
  ~texture2D();

  // write data |_data| to store for miplevel |_level|
  void write(const rx_byte* _data, rx_size _level);

  // map data for miplevel |_level|
  rx_byte* map(rx_size _level);

  void record_dimensions(const dimension_type& _dimensions);
  void record_wrap(const wrap_options& _wrap);

  const dimension_type& dimensions() const &;
  const wrap_options& wrap() const &;
  rx_size levels() const;
  const level_info<dimension_type>& info_for_level(rx_size _index) const &;

private:
  dimension_type m_dimensions;
  wrap_options m_wrap;
  array<level_info<dimension_type>> m_levels;
};

struct texture3D : texture {
  using dimension_type = math::vec3z;
  using wrap_options = math::vec3<wrap_type>;

  texture3D(interface* _frontend);
  ~texture3D();

  // write 3D data |_data| to store for miplevel |_level|
  void write(const rx_byte* _data, rx_size _level);

  // map data for miplevel |_level|
  rx_byte* map(rx_size _level);

  void record_dimensions(const dimension_type& _dimensions);
  void record_wrap(const wrap_options& _wrap);

  const dimension_type& dimensions() const &;
  const wrap_options& wrap() const &;
  rx_size levels() const;
  const level_info<dimension_type>& info_for_level(rx_size _index) const &;

private:
  dimension_type m_dimensions;
  wrap_options m_wrap;
  array<level_info<dimension_type>> m_levels;
};

struct textureCM : texture {
  using dimension_type = math::vec2z;
  using wrap_options = math::vec2<wrap_type>;

  enum class face : rx_u8 {
    k_right,
    k_left,
    k_top,
    k_bottom,
    k_front,
    k_back
  };

  textureCM(interface* _frontend);
  ~textureCM();

  // write data |_data| for face |_face| to store for miplevel |_level|
  void write(const rx_byte* _data, face _face, rx_size _level);

  // map data for face |_face| for miplevel |_level|
  rx_byte* map(rx_size _level, face _facer);

  void record_dimensions(const dimension_type& _dimensions);
  void record_wrap(const wrap_options& _wrap);

  const dimension_type& dimensions() const &;
  const wrap_options& wrap() const &;
  rx_size levels() const;
  const level_info<dimension_type>& info_for_level(rx_size _index) const &;

private:
  dimension_type m_dimensions;
  wrap_options m_wrap;
  array<level_info<dimension_type>> m_levels;
};

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

inline bool texture::is_compressed() const {
  switch (m_format) {
  case data_format::k_rgba_u8:
    return false;
  case data_format::k_bgra_u8:
    return false;
  case data_format::k_rgba_f16:
    return false;
  case data_format::k_bgra_f16:
    return false;
  case data_format::k_d16:
    return false;
  case data_format::k_d24:
    return false;
  case data_format::k_d32:
    return false;
  case data_format::k_d32f:
    return false;
  case data_format::k_d24_s8:
    return false;
  case data_format::k_d32f_s8:
    return false;
  case data_format::k_s8:
    return false;
  case data_format::k_r_u8:
    return false;
  case data_format::k_dxt1:
    return true;
  case data_format::k_dxt5:
    return true;
  }
  RX_UNREACHABLE();
}

inline rx_f32 texture::byte_size_of_format(data_format _format) {
  switch (_format) {
  case data_format::k_rgba_u8:
    return 4.0f;
  case data_format::k_bgra_u8:
    return 4.0f;
  case data_format::k_rgba_f16:
    return 8.0f;
  case data_format::k_bgra_f16:
    return 8.0f;
  case data_format::k_d16:
    return 2.0f;
  case data_format::k_d24:
    return 3.0f;
  case data_format::k_d32:
    return 4.0f;
  case data_format::k_d32f:
    return 4.0f;
  case data_format::k_d24_s8:
    return 4.0f;
  case data_format::k_d32f_s8:
    return 5.0f;
  case data_format::k_s8:
    return 1.0f;
  case data_format::k_r_u8:
    return 1.0f;
  case data_format::k_dxt1:
    return 0.5f;
  case data_format::k_dxt5:
    return 1.0f;
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
  case data_format::k_dxt1:
    return 3;
  case data_format::k_dxt5:
    return 4;
  }
  return 0;
}

// texture1D
inline const texture1D::dimension_type& texture1D::dimensions() const & {
  return m_dimensions;
}

inline const texture1D::wrap_options& texture1D::wrap() const & {
  return m_wrap;
}

inline rx_size texture1D::levels() const {
  RX_ASSERT(m_recorded & k_dimensions, "dimensions not recorded");
  RX_ASSERT(m_recorded & k_format, "format not recorded");

  if (m_filter.mipmaps) {
    const auto count{math::log2(m_dimensions) + 1};
    return is_compressed() ? count - 2 : count;
  }
  return 1;
}

inline const texture::level_info<texture1D::dimension_type>&
texture1D::info_for_level(rx_size _index) const & {
  return m_levels[_index];
}

// texture2D
inline const texture2D::dimension_type& texture2D::dimensions() const & {
  return m_dimensions;
}

inline const texture2D::wrap_options& texture2D::wrap() const & {
  return m_wrap;
}

inline rx_size texture2D::levels() const {
  RX_ASSERT(m_recorded & k_dimensions, "dimensions not recorded");
  RX_ASSERT(m_recorded & k_format, "format not recorded");

  if (m_filter.mipmaps) {
    const auto count{math::log2(m_dimensions.max_element()) + 1};
    return is_compressed() ? count - 2 : count;
  }
  return 1;
}

inline const texture::level_info<texture2D::dimension_type>&
texture2D::info_for_level(rx_size _index) const & {
  return m_levels[_index];
}

// texture3D
inline const texture3D::dimension_type& texture3D::dimensions() const & {
  return m_dimensions;
}

inline const texture3D::wrap_options& texture3D::wrap() const & {
  return m_wrap;
}

inline rx_size texture3D::levels() const {
  RX_ASSERT(m_recorded & k_dimensions, "dimensions not recorded");
  RX_ASSERT(m_recorded & k_format, "format not recorded");

  if (m_filter.mipmaps) {
    const auto count{math::log2(m_dimensions.max_element()) + 1};
    return is_compressed() ? count - 2 : count;
  }
  return 1;
}

inline const texture::level_info<texture3D::dimension_type>&
texture3D::info_for_level(rx_size _index) const & {
  return m_levels[_index];
}

// textureCM
inline const textureCM::dimension_type& textureCM::dimensions() const & {
  return m_dimensions;
}

inline const textureCM::wrap_options& textureCM::wrap() const & {
  return m_wrap;
}

inline rx_size textureCM::levels() const {
  RX_ASSERT(m_recorded & k_dimensions, "dimensions not recorded");
  RX_ASSERT(m_recorded & k_format, "format not recorded");
  
  if (m_filter.mipmaps) {
    const auto count{math::log2(m_dimensions.max_element()) + 1};
    return is_compressed() ? count - 2 : count;
  }
  return 1;
}

inline const texture::level_info<textureCM::dimension_type>&
textureCM::info_for_level(rx_size _index) const & {
  return m_levels[_index];
}

} // namespace rx::render::frontend

#endif // RX_RENDER_FRONTEND_TEXTURE_H