#ifndef RX_RENDER_FRONTEND_TEXTURE_H
#define RX_RENDER_FRONTEND_TEXTURE_H
#include "rx/render/frontend/resource.h"

#include "rx/core/vector.h"
#include "rx/core/algorithm/max.h"
#include "rx/core/math/log2.h"

#include "rx/math/vec2.h"
#include "rx/math/vec3.h"
#include "rx/math/vec4.h"

namespace Rx::Render::Frontend {

struct Context;

struct Texture
  : Resource
{
  Texture(Context* _frontend, Resource::Type _type);

  template<typename T>
  struct LevelInfo {
    Size offset;
    Size size;
    T dimensions;
  };

  struct FilterOptions {
    bool bilinear;
    bool trilinear;
    bool mipmaps;
  };

  enum class WrapType : Uint8 {
    k_clamp_to_edge,
    k_clamp_to_border,
    k_mirrored_repeat,
    k_mirror_clamp_to_edge,
    k_repeat
  };

  enum class DataFormat : Uint8 {
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

  enum class Type : Uint8 {
    ATTACHMENT,
    STATIC,
    DYNAMIC
  };

  // get byte size for one pixel of |_format|
  // NOTE: can be fractional for compressed block formats
  static Size bits_per_pixel(DataFormat _format);

  // get channel count of |_format|
  static Size channel_count_of_format(DataFormat _format);

  void record_format(DataFormat _format);
  void record_type(Type _type);
  void record_filter(const FilterOptions& _options);
  void record_levels(Size _levels); // the number of levels including the base level
  void record_border(const Math::Vec4f& _color);

  void validate() const;

  const Vector<Byte>& data() const &;
  DataFormat format() const;
  FilterOptions filter() const;
  Size channels() const;
  Type type() const;
  Size levels() const;
  const Math::Vec4f& border() const &;

  static bool is_compressed_format(DataFormat _format);
  static bool is_color_format(DataFormat _format);
  static bool is_depth_format(DataFormat _format);
  static bool is_stencil_format(DataFormat _format);
  static bool is_depth_stencil_format(DataFormat _format);
  static bool is_srgb_color_format(DataFormat _format);

  bool is_compressed_format() const;
  bool is_color_format() const;
  bool is_depth_format() const;
  bool is_stencil_format() const;
  bool is_depth_stencil_format() const;
  bool is_srgb_color_format() const;

  bool is_swapchain() const;
  bool is_level_in_range(Size _level) const;

  template<typename T>
  struct Edit {
    Size level;
    T offset;
    T size;
  };

protected:
  enum : Uint8 {
    k_format     = 1 << 0,
    k_type       = 1 << 1,
    k_filter     = 1 << 2,
    k_wrap       = 1 << 3,
    k_dimensions = 1 << 4,
    k_swapchain  = 1 << 5,
    k_levels     = 1 << 6,
    k_border     = 1 << 7
  };

  friend struct Context;

  Vector<Byte> m_data;
  DataFormat m_format;
  Type m_type;
  FilterOptions m_filter;
  Uint16 m_flags;
  Size m_levels;
  Math::Vec4f m_border;
};

struct Texture1D : Texture {
  using DimensionType = Size;
  using WrapOptions = WrapType;
  using EditType = Edit<DimensionType>;
  using LevelInfoType = LevelInfo<DimensionType>;

  Texture1D(Context* _frontend);
  ~Texture1D();

  // write data |_data| to store for miplevel |_level|
  void write(const Byte* _data, Size _level);

  // map data for miplevel |_level|
  Byte* map(Size _level);

  void record_dimensions(const DimensionType& _dimensions);
  void record_wrap(const WrapOptions& _wrap);

  const DimensionType& dimensions() const &;
  const WrapOptions& wrap() const &;
  const LevelInfoType& info_for_level(Size _index) const &;

  // Record an edit to level |_level| of this texture at offset |_offset| of
  // dimensions |_dimensions|.
  void record_edit(Size _level, const DimensionType& _offset,
    const DimensionType& _dimensions);

  const Vector<EditType>& edits() const;
  Size bytes_for_edits() const;
  void optimize_edits();
  void clear_edits();

private:
  DimensionType m_dimensions;
  WrapOptions m_wrap;
  Vector<LevelInfoType> m_level_info;
  Vector<EditType> m_edits;
};

struct Texture2D : Texture {
  using DimensionType = Math::Vec2z;
  using WrapOptions = Math::Vec2<WrapType>;
  using EditType = Edit<DimensionType>;
  using LevelInfoType = LevelInfo<DimensionType>;

  Texture2D(Context* _frontend);
  ~Texture2D();

  // write data |_data| to store for miplevel |_level|
  void write(const Byte* _data, Size _level);

  // map data for miplevel |_level|
  Byte* map(Size _level);

  void record_dimensions(const DimensionType& _dimensions);
  void record_wrap(const WrapOptions& _wrap);

  const DimensionType& dimensions() const &;
  const WrapOptions& wrap() const &;
  const LevelInfoType& info_for_level(Size _index) const &;

  // Record an edit to level |_level| of this texture at offset |_offset| of
  // dimensions |_dimensions|.
  void record_edit(Size _level, const DimensionType& _offset,
    const DimensionType& _dimensions);

  const Vector<EditType>& edits() const;
  Size bytes_for_edits() const;
  void optimize_edits();
  void clear_edits();

private:
  friend struct Context; // TODO(dweiler): Remove

  DimensionType m_dimensions;
  WrapOptions m_wrap;
  Vector<LevelInfo<DimensionType>> m_level_info;
  Vector<Edit<DimensionType>> m_edits;
};

struct Texture3D : Texture {
  using DimensionType = Math::Vec3z;
  using WrapOptions = Math::Vec3<WrapType>;
  using EditType = Edit<DimensionType>;
  using LevelInfoType = LevelInfo<DimensionType>;

  Texture3D(Context* _frontend);
  ~Texture3D();

  // write 3D data |_data| to store for miplevel |_level|
  void write(const Byte* _data, Size _level);

  // map data for miplevel |_level|
  Byte* map(Size _level);

  void record_dimensions(const DimensionType& _dimensions);
  void record_wrap(const WrapOptions& _wrap);

  const DimensionType& dimensions() const &;
  const WrapOptions& wrap() const &;
  const LevelInfoType& info_for_level(Size _index) const &;

  // Record an edit to level |_level| of this texture at offset |_offset| with
  // dimensions |_dimensions|.
  void record_edit(Size _level, const DimensionType& _offset,
    const DimensionType& _dimensions);

  // The number of bytes of texture data needed for edits.
  const Vector<EditType>& edits() const;
  Size bytes_for_edits() const;
  void optimize_edits();
  void clear_edits();

private:
  DimensionType m_dimensions;
  WrapOptions m_wrap;
  Vector<LevelInfoType> m_level_info;
  Vector<EditType> m_edits;
};

struct TextureCM : Texture {
  using DimensionType = Math::Vec2z;
  using WrapOptions = Math::Vec3<WrapType>;
  using LevelInfoType = LevelInfo<DimensionType>;

  enum class Face : Uint8 {
    k_right,  // +x
    k_left,   // -x
    k_top,    // +y
    k_bottom, // -y
    k_front,  // +z
    k_back    // -z
  };

  TextureCM(Context* _frontend);
  ~TextureCM();

  // write data |_data| for face |_face| to store for miplevel |_level|
  void write(const Byte* _data, Face _face, Size _level);

  // map data for face |_face| for miplevel |_level|
  Byte* map(Size _level, Face _face);

  void record_dimensions(const DimensionType& _dimensions);
  void record_wrap(const WrapOptions& _wrap);

  const DimensionType& dimensions() const &;
  const WrapOptions& wrap() const &;
  const LevelInfoType& info_for_level(Size _index) const &;

private:
  DimensionType m_dimensions;
  WrapOptions m_wrap;
  Vector<LevelInfoType> m_level_info;
};

// texture
inline const Vector<Byte>& Texture::data() const & {
  return m_data;
}

inline Texture::DataFormat Texture::format() const {
  return m_format;
}

inline Texture::FilterOptions Texture::filter() const {
  return m_filter;
}

inline Size Texture::channels() const {
  return channel_count_of_format(m_format);
}

inline Texture::Type Texture::type() const {
  return m_type;
}

inline Size Texture::levels() const {
  return m_levels;
}

inline const Math::Vec4f& Texture::border() const & {
  // TODO(dweiler): Better method for texture borders...
  // RX_ASSERT(m_flags & k_border, "border not recorded");
  return m_border;
}

inline bool Texture::is_color_format(DataFormat _format) {
  return _format == DataFormat::k_r_u8
      || _format == DataFormat::k_rgb_u8
      || _format == DataFormat::k_rgba_u8
      || _format == DataFormat::k_bgr_u8
      || _format == DataFormat::k_bgra_u8
      || _format == DataFormat::k_rgba_f16
      || _format == DataFormat::k_bgra_f16
      || _format == DataFormat::k_dxt1
      || _format == DataFormat::k_dxt5
      || _format == DataFormat::k_srgb_u8
      || _format == DataFormat::k_srgba_u8;
}

inline bool Texture::is_depth_format(DataFormat _format) {
  return _format == DataFormat::k_d16
      || _format == DataFormat::k_d24
      || _format == DataFormat::k_d32
      || _format == DataFormat::k_d32f;
}

inline bool Texture::is_stencil_format(DataFormat _format) {
  return _format == DataFormat::k_s8;
}

inline bool Texture::is_depth_stencil_format(DataFormat _format) {
  return _format == DataFormat::k_d24_s8
      || _format == DataFormat::k_d32f_s8;
}

inline bool Texture::is_compressed_format(DataFormat _format) {
  return _format == DataFormat::k_dxt1
      || _format == DataFormat::k_dxt5;
}

inline bool Texture::is_srgb_color_format(DataFormat _format) {
  return _format == DataFormat::k_srgb_u8
      || _format == DataFormat::k_srgba_u8;
}

inline bool Texture::is_compressed_format() const {
  return is_compressed_format(m_format);
}

inline bool Texture::is_color_format() const {
  return is_color_format(m_format);
}

inline bool Texture::is_depth_format() const {
  return is_depth_format(m_format);
}

inline bool Texture::is_stencil_format() const {
  return is_stencil_format(m_format);
}

inline bool Texture::is_depth_stencil_format() const {
  return is_depth_stencil_format(m_format);
}

inline bool Texture::is_srgb_color_format() const {
  return is_srgb_color_format(m_format);
}

inline bool Texture::is_swapchain() const {
  return m_flags & k_swapchain;
}

inline bool Texture::is_level_in_range(Size _level) const {
  return _level < m_levels;
}

inline Size Texture::bits_per_pixel(DataFormat _format) {
  switch (_format) {
  case DataFormat::k_rgba_u8:
    return 4 * 8;
  case DataFormat::k_rgb_u8:
    return 3 * 8;
  case DataFormat::k_bgra_u8:
    return 4 * 8;
  case DataFormat::k_bgr_u8:
    return 3 * 8;
  case DataFormat::k_rgba_f16:
    return 4 * 16;
  case DataFormat::k_bgra_f16:
    return 4 * 16;
  case DataFormat::k_d16:
    return 16;
  case DataFormat::k_d24:
    return 24;
  case DataFormat::k_d32:
    return 32;
  case DataFormat::k_d32f:
    return 32;
  case DataFormat::k_d24_s8:
    return 32;
  case DataFormat::k_d32f_s8:
    return 40;
  case DataFormat::k_s8:
    return 8;
  case DataFormat::k_r_u8:
    return 8;
  case DataFormat::k_dxt1:
    return 4;
  case DataFormat::k_dxt5:
    return 8;
  case DataFormat::k_srgb_u8:
    return 24;
  case DataFormat::k_srgba_u8:
    return 32;
  }
  return 0;
}

inline Size Texture::channel_count_of_format(DataFormat _format) {
  switch (_format) {
  case DataFormat::k_rgba_u8:
    return 4;
  case DataFormat::k_rgb_u8:
    return 3;
  case DataFormat::k_bgra_u8:
    return 4;
  case DataFormat::k_bgr_u8:
    return 3;
  case DataFormat::k_rgba_f16:
    return 4;
  case DataFormat::k_bgra_f16:
    return 4;
  case DataFormat::k_d16:
    return 1;
  case DataFormat::k_d24:
    return 1;
  case DataFormat::k_d32:
    return 1;
  case DataFormat::k_d32f:
    return 1;
  case DataFormat::k_d24_s8:
    return 2;
  case DataFormat::k_d32f_s8:
    return 2;
  case DataFormat::k_s8:
    return 1;
  case DataFormat::k_r_u8:
    return 1;
  case DataFormat::k_dxt1:
    return 3;
  case DataFormat::k_dxt5:
    return 4;
  case DataFormat::k_srgb_u8:
    return 3;
  case DataFormat::k_srgba_u8:
    return 4;
  }
  return 0;
}

// Texture1D
inline const Texture1D::DimensionType& Texture1D::dimensions() const & {
  return m_dimensions;
}

inline const Texture1D::WrapOptions& Texture1D::wrap() const & {
  return m_wrap;
}

inline const Texture1D::LevelInfoType& Texture1D::info_for_level(Size _index) const & {
  return m_level_info[_index];
}

inline const Vector<Texture1D::EditType>& Texture1D::edits() const {
  return m_edits;
}

inline void Texture1D::clear_edits() {
  m_edits.clear();
}

// Texture2D
inline const Texture2D::DimensionType& Texture2D::dimensions() const & {
  return m_dimensions;
}

inline const Texture2D::WrapOptions& Texture2D::wrap() const & {
  return m_wrap;
}

inline const Texture2D::LevelInfoType& Texture2D::info_for_level(Size _index) const & {
  return m_level_info[_index];
}

inline const Vector<Texture2D::EditType>& Texture2D::edits() const {
  return m_edits;
}

inline void Texture2D::clear_edits() {
  m_edits.clear();
}

// Texture3D
inline const Texture3D::DimensionType& Texture3D::dimensions() const & {
  return m_dimensions;
}

inline const Texture3D::WrapOptions& Texture3D::wrap() const & {
  return m_wrap;
}

inline const Texture3D::LevelInfoType& Texture3D::info_for_level(Size _index) const & {
  return m_level_info[_index];
}

inline const Vector<Texture3D::EditType>& Texture3D::edits() const {
  return m_edits;
}

inline void Texture3D::clear_edits() {
  m_edits.clear();
}

// TextureCM
inline const TextureCM::DimensionType& TextureCM::dimensions() const & {
  return m_dimensions;
}

inline const TextureCM::WrapOptions& TextureCM::wrap() const & {
  return m_wrap;
}

inline const TextureCM::LevelInfoType& TextureCM::info_for_level(Size _index) const & {
  return m_level_info[_index];
}

} // namespace rx::render::frontend

#endif // RX_RENDER_FRONTEND_TEXTURE_H
