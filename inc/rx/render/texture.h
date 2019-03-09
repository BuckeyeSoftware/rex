#ifndef RX_RENDER_TEXTURE_H
#define RX_RENDER_TEXTURE_H

#include <rx/render/resource.h>

#include <rx/core/array.h>

#include <rx/math/vec2.h>
#include <rx/math/vec3.h>
#include <rx/math/vec4.h>

namespace rx::render {

struct texture : resource {
  texture(resource::category _type);

  struct filter_options {
    bool blinear;
    bool trilinear;
    bool mip_maps;
  };

  struct wrap_options {
    enum class category {
      k_clamp_to_edge,
      k_clamp_to_border,
      k_mirrored_repeat,
      k_repeat
    };
    category s;
    category t;
    category r;
  };

  enum class data_format {
    k_rgba_u8,
    k_bgra_u8,
    k_d16,
    k_d24,
    k_d32,
    k_d32f,
    k_d24_s8,
    k_d32f_s8,
    k_s8
  };

  static rx_size byte_size_of_format(data_format _format);
  static rx_size channel_count_of_format(data_format _format);

  // record format |_format|
  void record_format(data_format _format);

  // record |_wrap| parameters
  void record_wrap(const wrap_options& _options);

  // record |_filter| parameters
  void record_filter(const filter_options& _options);

  const array<rx_byte>& data() const &;

  data_format format() const;
  filter_options filter() const;
  wrap_options wrap() const;
  rx_size channels() const;

protected:
  enum {
    k_format = 1 << 0,
    k_filter = 1 << 1,
    k_wrap = 1 << 2
  };

  array<rx_byte> m_data;
  data_format m_format;
  filter_options m_filter;
  wrap_options m_wrap;
  int m_recorded;
};

struct texture1D : texture {
  texture1D();
  ~texture1D();

  // write data |_data| of dimensions |_dimensions| to store
  void write(const rx_byte* _data, rx_size _dimensions);
  rx_size dimensions() const;
private:
  rx_size m_dimensions;
};

struct texture2D : texture {
  texture2D();
  ~texture2D();

  // write data |_data| of dimensions |_dimensions| to store
  void write(const rx_byte* _data, const math::vec2z& _dimensions);
  const math::vec2z& dimensions() const &;
private:
  math::vec2z m_dimensions;
};

struct texture3D : texture {
  texture3D();
  ~texture3D();

  // write data |_data| of dimensions |_dimensions| to store
  void write(const rx_byte* _data, const math::vec3z& _dimensions);
  const math::vec3z& dimensions() const &;
private:
  math::vec3z m_dimensions;
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

  textureCM();
  ~textureCM();

  // write data |_data| of dimensions |_dimensions| to store |_face|
  void write(const rx_byte* _data, const math::vec2z& _dimensions, face _face);
  const math::vec2z& dimensions() const &;
private:
  math::vec2z m_dimensions;
};

inline rx_size texture1D::dimensions() const {
  return m_dimensions;
}

inline const math::vec2z& texture2D::dimensions() const & {
  return m_dimensions;
}

inline const math::vec3z& texture3D::dimensions() const & {
  return m_dimensions;
}

inline const math::vec2z& textureCM::dimensions() const & {
  return m_dimensions;
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

inline texture::wrap_options texture::wrap() const {
  return m_wrap;
}

inline rx_size texture::byte_size_of_format(data_format _format) {
  switch (_format) {
  case data_format::k_rgba_u8:
    return 4;
  case data_format::k_bgra_u8:
    return 4;
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
  }
  return 0;
}

inline rx_size texture::channel_count_of_format(data_format _format) {
  switch (_format) {
  case data_format::k_rgba_u8:
    return 4;
  case data_format::k_bgra_u8:
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
  }
  return 0;
}

} // namespace rx::render

#endif // RX_RENDER_TEXTURE_H
