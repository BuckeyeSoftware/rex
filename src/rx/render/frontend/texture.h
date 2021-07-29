#ifndef RX_RENDER_FRONTEND_TEXTURE_H
#define RX_RENDER_FRONTEND_TEXTURE_H
#include "rx/render/frontend/resource.h"

#include "rx/core/vector.h"
#include "rx/core/linear_buffer.h"
#include "rx/core/algorithm/max.h"
#include "rx/core/math/log2.h"
#include "rx/core/hints/unreachable.h"

#include "rx/math/vec2.h"
#include "rx/math/vec3.h"
#include "rx/math/vec4.h"

namespace Rx::Render::Frontend {

struct Context;

struct Texture
  : Resource
{
  RX_MARK_NO_COPY(Texture);
  RX_MARK_NO_MOVE(Texture);

  Texture(Context* _frontend, Resource::Type _type);

  template<typename T>
  struct LevelInfoType {
    Size offset;
    Size size;
    T dimensions;
  };

  enum class DataFormat : Uint8 {
    R_U8,
    R_F16,
    RG_F16,
    RGB_U8,
    RGBA_U8,
    BGR_U8,
    BGRA_U8,
    RGBA_F16,
    BGRA_F16,
    RGBA_F32,
    D16,
    D24,
    D32,
    D32F,
    D24_S8,
    D32F_S8,
    S8,
    DXT1,
    DXT5,
    SRGB_U8,
    SRGBA_U8
  };

  enum class Type : Uint8 {
    ATTACHMENT,
    STATIC,
    DYNAMIC
  };

  void record_format(DataFormat _format);
  void record_type(Type _type);
  void record_levels(Size _levels); // the number of levels including the base level

  void validate() const;

  const LinearBuffer& data() const &;
  DataFormat format() const;
  Size channels() const;
  Type type() const;
  Size levels() const;

  Size bits_per_pixel() const;

  static bool is_compressed_format(DataFormat _format);
  static bool is_color_format(DataFormat _format);
  static bool is_depth_format(DataFormat _format);
  static bool is_stencil_format(DataFormat _format);
  static bool is_depth_stencil_format(DataFormat _format);
  static bool is_srgb_color_format(DataFormat _format);
  static bool is_renderable(DataFormat _format);
  static bool has_alpha(DataFormat _format);

  bool is_compressed_format() const;
  bool is_color_format() const;
  bool is_depth_format() const;
  bool is_stencil_format() const;
  bool is_depth_stencil_format() const;
  bool is_srgb_color_format() const;
  bool is_renderable() const;

  bool has_alpha() const;

  bool is_swapchain() const;
  bool is_level_in_range(Size _level) const;

  template<typename T>
  struct EditType {
    Size level;
    T offset;
    T size;
  };

protected:
  enum : Uint8 {
    FORMAT     = 1 << 0,
    TYPE       = 1 << 1,
    DIMENSIONS = 1 << 2,
    SWAPCHAIN  = 1 << 3,
    LEVELS     = 1 << 4
  };

  friend struct Context;

  LinearBuffer m_data;
  DataFormat m_format;
  Type m_type;
  Uint16 m_flags;
  Size m_levels;
};

struct Texture1D : Texture {
  using DimensionType = Size;
  using Edit = EditType<DimensionType>;
  using LevelInfo = LevelInfoType<DimensionType>;

  Texture1D(Context* _frontend);
  ~Texture1D();

  // write data |_data| to store for miplevel |_level|
  void write(const Byte* _data, Size _level);

  // map data for miplevel |_level|
  Byte* map(Size _level);

  bool record_dimensions(const DimensionType& _dimensions);

  const DimensionType& dimensions() const &;
  const LevelInfo& info_for_level(Size _index) const &;

  // Record an edit to level |_level| of this texture at offset |_offset| of
  // dimensions |_dimensions|.
  void record_edit(Size _level, const DimensionType& _offset,
    const DimensionType& _dimensions);

  const Vector<Edit>& edits() const;
  Size bytes_for_edits() const;
  void clear_edits();

private:
  DimensionType m_dimensions;
  Vector<LevelInfo> m_level_info;
  Vector<Edit> m_edits;
};

struct Texture2D : Texture {
  using DimensionType = Math::Vec2z;
  using Edit = EditType<DimensionType>;
  using LevelInfo = LevelInfoType<DimensionType>;

  Texture2D(Context* _frontend);
  ~Texture2D();

  // write data |_data| to store for miplevel |_level|
  void write(const Byte* _data, Size _level);

  // map data for miplevel |_level|
  Byte* map(Size _level);

  bool record_dimensions(const DimensionType& _dimensions);

  const DimensionType& dimensions() const &;
  const LevelInfo& info_for_level(Size _index) const &;

  // Record an edit to level |_level| of this texture at offset |_offset| of
  // dimensions |_dimensions|.
  void record_edit(Size _level, const DimensionType& _offset,
    const DimensionType& _dimensions);

  const Vector<Edit>& edits() const;
  Size bytes_for_edits() const;
  void clear_edits();

private:
  friend struct Context; // TODO(dweiler): Remove

  DimensionType m_dimensions;
  Vector<LevelInfo> m_level_info;
  Vector<Edit> m_edits;
};

struct Texture3D : Texture {
  using DimensionType = Math::Vec3z;
  using Edit = EditType<DimensionType>;
  using LevelInfo = LevelInfoType<DimensionType>;

  Texture3D(Context* _frontend);
  ~Texture3D();

  // write 3D data |_data| to store for miplevel |_level|
  void write(const Byte* _data, Size _level);

  // map data for miplevel |_level|
  Byte* map(Size _level);

  bool record_dimensions(const DimensionType& _dimensions);

  const DimensionType& dimensions() const &;
  const LevelInfo& info_for_level(Size _index) const &;

  // Record an edit to level |_level| of this texture at offset |_offset| with
  // dimensions |_dimensions|.
  void record_edit(Size _level, const DimensionType& _offset,
    const DimensionType& _dimensions);

  // The number of bytes of texture data needed for edits.
  const Vector<Edit>& edits() const;
  Size bytes_for_edits() const;
  void clear_edits();

private:
  DimensionType m_dimensions;
  Vector<LevelInfo> m_level_info;
  Vector<Edit> m_edits;
};

struct TextureCM : Texture {
  using DimensionType = Math::Vec2z;
  using LevelInfo = LevelInfoType<DimensionType>;

  enum class Face : Uint8 {
    RIGHT,  // +x
    LEFT,   // -x
    TOP,    // +y
    BOTTOM, // -y
    FRONT,  // +z
    BACK    // -z
  };

  TextureCM(Context* _frontend);
  ~TextureCM();

  // write data |_data| for face |_face| to store for miplevel |_level|
  void write(const Byte* _data, Face _face, Size _level);

  // map data for face |_face| for miplevel |_level|
  Byte* map(Size _level, Face _face);

  bool record_dimensions(const DimensionType& _dimensions);

  const DimensionType& dimensions() const &;
  const LevelInfo& info_for_level(Size _index) const &;

private:
  DimensionType m_dimensions;
  Vector<LevelInfo> m_level_info;
};

// Texture
inline const LinearBuffer& Texture::data() const & {
  return m_data;
}

inline Texture::DataFormat Texture::format() const {
  RX_ASSERT(m_flags & FORMAT, "format not recorded");
  return m_format;
}

inline Size Texture::channels() const {
  switch (m_format) {
  case DataFormat::RGBA_U8:
    return 4;
  case DataFormat::RGB_U8:
    return 3;
  case DataFormat::BGRA_U8:
    return 4;
  case DataFormat::BGR_U8:
    return 3;
  case DataFormat::RGBA_F16:
    return 4;
  case DataFormat::BGRA_F16:
    return 4;
  case DataFormat::RGBA_F32:
    return 4;
  case DataFormat::D16:
    return 1;
  case DataFormat::D24:
    return 1;
  case DataFormat::D32:
    return 1;
  case DataFormat::D32F:
    return 1;
  case DataFormat::D24_S8:
    return 2;
  case DataFormat::D32F_S8:
    return 2;
  case DataFormat::S8:
    return 1;
  case DataFormat::R_U8:
    return 1;
  case DataFormat::R_F16:
    return 1;
  case DataFormat::RG_F16:
    return 2;
  case DataFormat::DXT1:
    return 3;
  case DataFormat::DXT5:
    return 4;
  case DataFormat::SRGB_U8:
    return 3;
  case DataFormat::SRGBA_U8:
    return 4;
  }
  RX_HINT_UNREACHABLE();
}

inline Texture::Type Texture::type() const {
  return m_type;
}

inline Size Texture::levels() const {
  return m_levels;
}

inline Size Texture::bits_per_pixel() const {
  switch (m_format) {
  case DataFormat::RGBA_U8:
    return 4 * 8;
  case DataFormat::RGB_U8:
    return 3 * 8;
  case DataFormat::BGRA_U8:
    return 4 * 8;
  case DataFormat::BGR_U8:
    return 3 * 8;
  case DataFormat::RGBA_F16:
    return 4 * 16;
  case DataFormat::BGRA_F16:
    return 4 * 16;
  case DataFormat::RGBA_F32:
    return 4 * 32;
  case DataFormat::D16:
    return 16;
  case DataFormat::D24:
    return 24;
  case DataFormat::D32:
    return 32;
  case DataFormat::D32F:
    return 32;
  case DataFormat::D24_S8:
    return 32;
  case DataFormat::D32F_S8:
    return 40;
  case DataFormat::S8:
    return 8;
  case DataFormat::R_U8:
    return 8;
  case DataFormat::R_F16:
    return 16;
  case DataFormat::RG_F16:
    return 8 * 2 * 2;
  case DataFormat::DXT1:
    return 4;
  case DataFormat::DXT5:
    return 8;
  case DataFormat::SRGB_U8:
    return 24;
  case DataFormat::SRGBA_U8:
    return 32;
  }
  RX_HINT_UNREACHABLE();
}

inline bool Texture::has_alpha() const {
  return has_alpha(m_format);
}

inline bool Texture::is_color_format(DataFormat _format) {
  return _format == DataFormat::R_U8
      || _format == DataFormat::R_F16
      || _format == DataFormat::RG_F16
      || _format == DataFormat::RGB_U8
      || _format == DataFormat::RGBA_U8
      || _format == DataFormat::BGR_U8
      || _format == DataFormat::BGRA_U8
      || _format == DataFormat::RGBA_F16
      || _format == DataFormat::BGRA_F16
      || _format == DataFormat::RGBA_F32
      || _format == DataFormat::DXT1
      || _format == DataFormat::DXT5
      || _format == DataFormat::SRGB_U8
      || _format == DataFormat::SRGBA_U8;
}

inline bool Texture::is_depth_format(DataFormat _format) {
  return _format == DataFormat::D16
      || _format == DataFormat::D24
      || _format == DataFormat::D32
      || _format == DataFormat::D32F;
}

inline bool Texture::is_stencil_format(DataFormat _format) {
  return _format == DataFormat::S8;
}

inline bool Texture::is_depth_stencil_format(DataFormat _format) {
  return _format == DataFormat::D24_S8
      || _format == DataFormat::D32F_S8;
}

inline bool Texture::is_compressed_format(DataFormat _format) {
  return _format == DataFormat::DXT1
      || _format == DataFormat::DXT5;
}

inline bool Texture::is_srgb_color_format(DataFormat _format) {
  return _format == DataFormat::SRGB_U8
      || _format == DataFormat::SRGBA_U8;
}

inline bool Texture::is_renderable(DataFormat _format) {
  // Cannot render to compressed texutures, 3-channel textures, depth, or
  // stencil textures.
  switch (_format) {
  case DataFormat::R_U8:
    return true;
  case DataFormat::R_F16:
    return true;
  case DataFormat::RG_F16:
    return true;
  case DataFormat::RGB_U8:
    return false;
  case DataFormat::RGBA_U8:
    return true;
  case DataFormat::BGR_U8:
    return false;
  case DataFormat::BGRA_U8:
    return false;
  case DataFormat::RGBA_F16:
    return true;
  case DataFormat::BGRA_F16:
    return false;
  case DataFormat::RGBA_F32:
    return true;
  case DataFormat::D16:
    return false;
  case DataFormat::D24:
    return false;
  case DataFormat::D32:
    return false;
  case DataFormat::D32F:
    return false;
  case DataFormat::D24_S8:
    return false;
  case DataFormat::D32F_S8:
    return false;
  case DataFormat::S8:
    return false;
  case DataFormat::DXT1:
    return false;
  case DataFormat::DXT5:
    return false;
  case DataFormat::SRGB_U8:
    return false;
  case DataFormat::SRGBA_U8:
    return true;
  }
  RX_HINT_UNREACHABLE();
}

inline bool Texture::has_alpha(DataFormat _format) {
  switch (_format) {
  case DataFormat::R_U8:
    return false;
  case DataFormat::R_F16:
    return false;
  case DataFormat::RG_F16:
    return false;
  case DataFormat::RGB_U8:
    return false;
  case DataFormat::RGBA_U8:
    return true;
  case DataFormat::BGR_U8:
    return false;
  case DataFormat::BGRA_U8:
    return true;
  case DataFormat::RGBA_F16:
    return true;
  case DataFormat::BGRA_F16:
    return true;
  case DataFormat::RGBA_F32:
    return true;
  case DataFormat::D16:
    return false;
  case DataFormat::D24:
    return false;
  case DataFormat::D32:
    return false;
  case DataFormat::D32F:
    return false;
  case DataFormat::D24_S8:
    return false;
  case DataFormat::D32F_S8:
    return false;
  case DataFormat::S8:
    return false;
  case DataFormat::DXT1:
    return false;
  case DataFormat::DXT5:
    return false;
  case DataFormat::SRGB_U8:
    return false;
  case DataFormat::SRGBA_U8:
    return true;
  }
  RX_HINT_UNREACHABLE();
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

inline bool Texture::is_renderable() const {
  return is_renderable(m_format);
}

inline bool Texture::is_swapchain() const {
  return m_flags & SWAPCHAIN;
}

inline bool Texture::is_level_in_range(Size _level) const {
  return _level < m_levels;
}

// Texture1D
inline const Texture1D::DimensionType& Texture1D::dimensions() const & {
  return m_dimensions;
}

inline const Texture1D::LevelInfo& Texture1D::info_for_level(Size _index) const & {
  return m_level_info[_index];
}

inline const Vector<Texture1D::Edit>& Texture1D::edits() const {
  return m_edits;
}

inline void Texture1D::clear_edits() {
  m_edits.clear();
}

// Texture2D
inline const Texture2D::DimensionType& Texture2D::dimensions() const & {
  return m_dimensions;
}

inline const Texture2D::LevelInfo& Texture2D::info_for_level(Size _index) const & {
  return m_level_info[_index];
}

inline const Vector<Texture2D::Edit>& Texture2D::edits() const {
  return m_edits;
}

inline void Texture2D::clear_edits() {
  m_edits.clear();
}

// Texture3D
inline const Texture3D::DimensionType& Texture3D::dimensions() const & {
  return m_dimensions;
}

inline const Texture3D::LevelInfo& Texture3D::info_for_level(Size _index) const & {
  return m_level_info[_index];
}

inline const Vector<Texture3D::Edit>& Texture3D::edits() const {
  return m_edits;
}

inline void Texture3D::clear_edits() {
  m_edits.clear();
}

// TextureCM
inline const TextureCM::DimensionType& TextureCM::dimensions() const & {
  return m_dimensions;
}

inline const TextureCM::LevelInfo& TextureCM::info_for_level(Size _index) const & {
  return m_level_info[_index];
}

} // namespace Rx::Render::Frontend

#endif // RX_RENDER_FRONTEND_TEXTURE_H
