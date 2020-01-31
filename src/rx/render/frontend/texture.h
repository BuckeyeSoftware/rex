#ifndef RX_RENDER_FRONTEND_TEXTURE_H
#define RX_RENDER_FRONTEND_TEXTURE_H
#include "rx/render/frontend/resource.h"

#include "rx/core/vector.h"
#include "rx/core/algorithm/max.h"
#include "rx/core/math/log2.h"

#include "rx/math/vec2.h"
#include "rx/math/vec3.h"
#include "rx/math/vec4.h"

namespace rx::render::frontend {

struct interface;

struct texture
  : resource
{
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
    k_rgb_u8,
    k_rgba_u8,
    k_bgr_u8,
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
    k_dxt5,
    k_srgb_u8,
    k_srgba_u8
  };

  enum class type : rx_u8 {
    k_attachment,
    k_static,
    k_dynamic
  };

  // get byte size for one pixel of |_format|
  // NOTE: can be fractional for compressed block formats
  static rx_size bits_per_pixel(data_format _format);

  // get channel count of |_format|
  static rx_size channel_count_of_format(data_format _format);

  void record_format(data_format _format);
  void record_type(type _type);
  void record_filter(const filter_options& _options);
  void record_levels(rx_size _levels); // the number of levels including the base level
  void record_border(const math::vec4f& _color);

  void validate() const;

  const vector<rx_byte>& data() const &;
  data_format format() const;
  filter_options filter() const;
  rx_size channels() const;
  type kind() const;
  rx_size levels() const;
  const math::vec4f& border() const &;

  static bool is_compressed_format(data_format _format);
  static bool is_color_format(data_format _format);
  static bool is_depth_format(data_format _format);
  static bool is_stencil_format(data_format _format);
  static bool is_depth_stencil_format(data_format _format);
  static bool is_srgb_color_format(data_format _format);

  bool is_compressed_format() const;
  bool is_color_format() const;
  bool is_depth_format() const;
  bool is_stencil_format() const;
  bool is_depth_stencil_format() const;
  bool is_srgb_color_format() const;

  bool is_swapchain() const;
  bool is_level_in_range(rx_size _level) const;

  template<typename T>
  struct edit {
    rx_size level;
    T offset;
    T size;
  };

protected:
  enum : rx_u8 {
    k_format     = 1 << 0,
    k_type       = 1 << 1,
    k_filter     = 1 << 2,
    k_wrap       = 1 << 3,
    k_dimensions = 1 << 4,
    k_swapchain  = 1 << 5,
    k_levels     = 1 << 6,
    k_border     = 1 << 7
  };

  friend struct interface;

  vector<rx_byte> m_data;
  data_format m_format;
  type m_type;
  filter_options m_filter;
  rx_u16 m_flags;
  rx_size m_levels;
  math::vec4f m_border;
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
  const level_info<dimension_type>& info_for_level(rx_size _index) const &;
  vector<edit<dimension_type>>&& edits();

  // Record an edit to level |_level| of this texture at offset |_offset| of
  // dimensions |_dimensions|.
  void record_edit(rx_size _level, const dimension_type& _offset,
    const dimension_type& _dimensions);

private:
  dimension_type m_dimensions;
  wrap_options m_wrap;
  vector<level_info<dimension_type>> m_level_info;
  vector<edit<dimension_type>> m_edits;
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
  const level_info<dimension_type>& info_for_level(rx_size _index) const &;
  vector<edit<dimension_type>>&& edits();

  // Record an edit to level |_level| of this texture at offset |_offset| of
  // dimensions |_dimensions|.
  void record_edit(rx_size _level, const dimension_type& _offset,
    const dimension_type& _dimensions);

private:
  friend struct interface;

  dimension_type m_dimensions;
  wrap_options m_wrap;
  vector<level_info<dimension_type>> m_level_info;
  vector<edit<dimension_type>> m_edits;
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
  const level_info<dimension_type>& info_for_level(rx_size _index) const &;
  vector<edit<dimension_type>>&& edits();

  // Record an edit to level |_level| of this texture at offset |_offset| with
  // dimensions |_dimensions|.
  void record_edit(rx_size _level, const dimension_type& _offset,
    const dimension_type& _dimensions);

private:
  dimension_type m_dimensions;
  wrap_options m_wrap;
  vector<level_info<dimension_type>> m_level_info;
  vector<edit<dimension_type>> m_edits;
};

struct textureCM : texture {
  using dimension_type = math::vec2z;
  using wrap_options = math::vec3<wrap_type>;

  enum class face : rx_u8 {
    k_right,  // +x
    k_left,   // -x
    k_top,    // +y
    k_bottom, // -y
    k_front,  // +z
    k_back    // -z
  };

  textureCM(interface* _frontend);
  ~textureCM();

  // write data |_data| for face |_face| to store for miplevel |_level|
  void write(const rx_byte* _data, face _face, rx_size _level);

  // map data for face |_face| for miplevel |_level|
  rx_byte* map(rx_size _level, face _face);

  void record_dimensions(const dimension_type& _dimensions);
  void record_wrap(const wrap_options& _wrap);

  const dimension_type& dimensions() const &;
  const wrap_options& wrap() const &;
  const level_info<dimension_type>& info_for_level(rx_size _index) const &;

private:
  dimension_type m_dimensions;
  wrap_options m_wrap;
  vector<level_info<dimension_type>> m_level_info;
};

// texture
inline const vector<rx_byte>& texture::data() const & {
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

inline rx_size texture::levels() const {
  return m_levels;
}

inline const math::vec4f& texture::border() const & {
  RX_ASSERT(m_flags & k_border, "border not recorded");
  return m_border;
}

inline bool texture::is_color_format(data_format _format) {
  return _format == data_format::k_r_u8
      || _format == data_format::k_rgb_u8
      || _format == data_format::k_rgba_u8
      || _format == data_format::k_bgr_u8
      || _format == data_format::k_bgra_u8
      || _format == data_format::k_rgba_f16
      || _format == data_format::k_bgra_f16
      || _format == data_format::k_dxt1
      || _format == data_format::k_dxt5
      || _format == data_format::k_srgb_u8
      || _format == data_format::k_srgba_u8;
}

inline bool texture::is_depth_format(data_format _format) {
  return _format == data_format::k_d16
      || _format == data_format::k_d24
      || _format == data_format::k_d32
      || _format == data_format::k_d32f;
}

inline bool texture::is_stencil_format(data_format _format) {
  return _format == data_format::k_s8;
}

inline bool texture::is_depth_stencil_format(data_format _format) {
  return _format == data_format::k_d24_s8
      || _format == data_format::k_d32f_s8;
}

inline bool texture::is_compressed_format(data_format _format) {
  return _format == data_format::k_dxt1
      || _format == data_format::k_dxt5;
}

inline bool texture::is_srgb_color_format(data_format _format) {
  return _format == data_format::k_srgb_u8
      || _format == data_format::k_srgba_u8;
}

inline bool texture::is_compressed_format() const {
  return is_compressed_format(m_format);
}

inline bool texture::is_color_format() const {
  return is_color_format(m_format);
}

inline bool texture::is_depth_format() const {
  return is_depth_format(m_format);
}

inline bool texture::is_stencil_format() const {
  return is_stencil_format(m_format);
}

inline bool texture::is_depth_stencil_format() const {
  return is_depth_stencil_format(m_format);
}

inline bool texture::is_srgb_color_format() const {
  return is_srgb_color_format(m_format);
}

inline bool texture::is_swapchain() const {
  return m_flags & k_swapchain;
}

inline bool texture::is_level_in_range(rx_size _level) const {
  return _level < m_levels;
}

inline rx_size texture::bits_per_pixel(data_format _format) {
  switch (_format) {
  case data_format::k_rgba_u8:
    return 4 * 8;
  case data_format::k_rgb_u8:
    return 3 * 8;
  case data_format::k_bgra_u8:
    return 4 * 8;
  case data_format::k_bgr_u8:
    return 3 * 8;
  case data_format::k_rgba_f16:
    return 4 * 16;
  case data_format::k_bgra_f16:
    return 4 * 16;
  case data_format::k_d16:
    return 16;
  case data_format::k_d24:
    return 24;
  case data_format::k_d32:
    return 32;
  case data_format::k_d32f:
    return 32;
  case data_format::k_d24_s8:
    return 32;
  case data_format::k_d32f_s8:
    return 40;
  case data_format::k_s8:
    return 8;
  case data_format::k_r_u8:
    return 8;
  case data_format::k_dxt1:
    return 4;
  case data_format::k_dxt5:
    return 8;
  case data_format::k_srgb_u8:
    return 24;
  case data_format::k_srgba_u8:
    return 32;
  }
  return 0;
}

inline rx_size texture::channel_count_of_format(data_format _format) {
  switch (_format) {
  case data_format::k_rgba_u8:
    return 4;
  case data_format::k_rgb_u8:
    return 3;
  case data_format::k_bgra_u8:
    return 4;
  case data_format::k_bgr_u8:
    return 3;
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
  case data_format::k_srgb_u8:
    return 3;
  case data_format::k_srgba_u8:
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

inline const texture::level_info<texture1D::dimension_type>&
texture1D::info_for_level(rx_size _index) const & {
  return m_level_info[_index];
}

// texture2D
inline const texture2D::dimension_type& texture2D::dimensions() const & {
  return m_dimensions;
}

inline const texture2D::wrap_options& texture2D::wrap() const & {
  return m_wrap;
}

inline const texture::level_info<texture2D::dimension_type>&
texture2D::info_for_level(rx_size _index) const & {
  return m_level_info[_index];
}

// texture3D
inline const texture3D::dimension_type& texture3D::dimensions() const & {
  return m_dimensions;
}

inline const texture3D::wrap_options& texture3D::wrap() const & {
  return m_wrap;
}

inline const texture::level_info<texture3D::dimension_type>&
texture3D::info_for_level(rx_size _index) const & {
  return m_level_info[_index];
}

// textureCM
inline const textureCM::dimension_type& textureCM::dimensions() const & {
  return m_dimensions;
}

inline const textureCM::wrap_options& textureCM::wrap() const & {
  return m_wrap;
}

inline const texture::level_info<textureCM::dimension_type>&
textureCM::info_for_level(rx_size _index) const & {
  return m_level_info[_index];
}

} // namespace rx::render::frontend

#endif // RX_RENDER_FRONTEND_TEXTURE_H
