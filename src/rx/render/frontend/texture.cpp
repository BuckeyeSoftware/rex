#include <string.h> // memcpy

#include "rx/render/frontend/texture.h"
#include "rx/render/frontend/context.h"

#include "rx/core/algorithm/quick_sort.h"

#include "rx/core/math/log2.h"

namespace Rx::Render::Frontend {

// When an edit is inside a larger edit, that larger edit will include the
// nested edit. Remove such edits (duplicate and fully overlapping) to produce a
// minimal and coalesced edit list for the backend.
template<typename T>
static void coalesce_edits(Vector<T>& edits_) {
  // Determines if |_lhs| is fully inside |_rhs|.
  auto inside = [](const T& _lhs, const T& _rhs) {
    return !(_lhs.offset < _rhs.offset || _lhs.offset + _lhs.size > _rhs.offset + _rhs.size);
  };

  // WARN(dweiler): This behaves O(n^2), except n should be small.
  for (Size i = 0; i < edits_.size(); i++) {
    for (Size j = 0; j < edits_.size(); j++) {
      if (i == j) {
        continue;
      }

      // Only when the edits are to the same level of the texture.
      const auto& e1{edits_[i]};
      const auto& e2{edits_[j]};
      if (e1.level != e2.level) {
        continue;
      }

      // Edit exists fully inside another, remove it.
      if (inside(e1, e2)) {
        edits_.erase(i, i + 1);
      }
    }
  }

  // Sort the edits by texture level so largest levels come first.
  Algorithm::quick_sort(&edits_.first(), &edits_.last(),
    [](const T& _lhs, const T& _rhs) {
      return _lhs.level > _rhs.level;
    });
}

Texture::Texture(Context* _frontend, Resource::Type _type)
  : Resource{_frontend, _type}
  , m_data{_frontend->allocator()}
  , m_flags{0}
{
}

void Texture::record_format(DataFormat _format) {
  RX_ASSERT(!(m_flags & k_format), "format already recorded");
  m_format = _format;
  m_flags |= k_format;
}

void Texture::record_type(Type _type) {
  RX_ASSERT(!(m_flags & k_type), "Type already recorded");
  m_type = _type;
  m_flags |= k_type;
}

void Texture::record_filter(const FilterOptions& _options) {
  RX_ASSERT(!(m_flags & k_filter), "filter already recorded");
  m_filter = _options;
  m_flags |= k_filter;
}

void Texture::record_levels(Size _levels) {
  RX_ASSERT(!(m_flags & k_levels), "levels already recorded");
  RX_ASSERT(_levels, "_levels must be at least 1");

  m_levels = _levels;
  m_flags |= k_levels;
}

void Texture::record_border(const Math::Vec4f& _border) {
  RX_ASSERT(!(m_flags & k_border), "border already recorded");
  RX_ASSERT(m_flags & k_wrap, "wrap not recorded");

  m_border = _border;
  m_flags |= k_border;
}

void Texture::validate() const {
  RX_ASSERT(m_flags & k_format, "format not recorded");
  RX_ASSERT(m_flags & k_type, "Type not recorded");
  RX_ASSERT(m_flags & k_filter, "filter not recorded");
  RX_ASSERT(m_flags & k_wrap, "wrap not recorded");
  RX_ASSERT(m_flags & k_dimensions, "dimensions not recorded");
  RX_ASSERT(m_flags & k_levels, "levels not recorded");

  if (m_filter.mipmaps) {
    RX_ASSERT(m_levels > 1, "no levels specified for mipmaps");
  }

  if (m_flags & k_swapchain) {
    RX_ASSERT(!m_filter.mipmaps, "swapchain cannot have mipmaps");
    RX_ASSERT(!m_filter.bilinear, "swapchain cannot have bilinear filtering");
    RX_ASSERT(!m_filter.trilinear, "swapchain cannot have trilinear filtering");
    RX_ASSERT(m_levels == 1, "swapchain cannot have levels");
  }
}

// Texture1D
Texture1D::Texture1D(Context* _frontend)
  : Texture{_frontend, Resource::Type::k_texture1D}
  , m_level_info{_frontend->allocator()}
  , m_edits{_frontend->allocator()}
{
}

Texture1D::~Texture1D() {
}

void Texture1D::write(const Byte* _data, Size _level) {
  RX_ASSERT(_data, "_data is null");
  RX_ASSERT(is_level_in_range(_level), "mipmap level out of bounds");

  const auto& info{info_for_level(_level)};
  memcpy(m_data.data() + info.offset, _data, info.size);
}

Byte* Texture1D::map(Size _level) {
  RX_ASSERT(is_level_in_range(_level), "mipmap level out of bounds");
  validate();

  const auto& info{info_for_level(_level)};
  return m_data.data() + info.offset;
}

void Texture1D::record_dimensions(const DimensionType& _dimensions) {
  RX_ASSERT(!(m_flags & k_dimensions), "dimensions already recorded");

  RX_ASSERT(m_flags & k_type, "Type not recorded");
  RX_ASSERT(m_flags & k_format, "format not recorded");
  RX_ASSERT(m_flags & k_levels, "levels not recorded");

  RX_ASSERT(!is_compressed_format(), "1D textures cannot be compressed");

  m_dimensions = _dimensions;
  m_flags |= k_dimensions;

  DimensionType dimensions{m_dimensions};
  Size offset{0};
  const auto bpp{bits_per_pixel(m_format)};
  for (Size i{0}; i < m_levels; i++) {
    const auto size{static_cast<Size>(dimensions * bpp / 8)};
    m_level_info.push_back({offset, size, dimensions});
    offset += size;
    dimensions = Algorithm::max(dimensions / 2, 1_z);
  }

  if (m_type != Type::k_attachment) {
    m_data.resize(offset, Utility::UninitializedTag{});
    update_resource_usage(m_data.size());
  }
}

void Texture1D::record_wrap(const WrapOptions& _wrap) {
  RX_ASSERT(!(m_flags & k_wrap), "wrap already recorded");

  m_wrap = _wrap;
  m_flags |= k_wrap;
}

void Texture1D::record_edit(Size _level, const DimensionType& _offset,
                            const DimensionType& _dimensions)
{
  RX_ASSERT(is_level_in_range(_level), "mipmap level out of bounds");
  m_edits.emplace_back(_level, _offset, _dimensions);
}

Size Texture1D::bytes_for_edits() const {
  Size bytes = 0;
  m_edits.each_fwd([&](const EditType& _edit) { bytes += _edit.size; });
  return bytes * bits_per_pixel(format()) / 8;
}

void Texture1D::optimize_edits() {
  coalesce_edits(m_edits);
}

// Texture2D
Texture2D::Texture2D(Context* _frontend)
  : Texture{_frontend, Resource::Type::k_texture2D}
  , m_level_info{_frontend->allocator()}
  , m_edits{_frontend->allocator()}
{
}

Texture2D::~Texture2D() {
}

void Texture2D::write(const Byte* _data, Size _level) {
  RX_ASSERT(_data, "_data is null");
  RX_ASSERT(is_level_in_range(_level), "mipmap level out of bounds");
  validate();

  const auto& info{info_for_level(_level)};
  memcpy(m_data.data() + info.offset, _data, info.size);
}

Byte* Texture2D::map(Size _level) {
  RX_ASSERT(is_level_in_range(_level), "mipmap level out of bounds");
  validate();

  const auto& info{info_for_level(_level)};
  return m_data.data() + info.offset;
}

void Texture2D::record_dimensions(const Math::Vec2z& _dimensions) {
  RX_ASSERT(!(m_flags & k_dimensions), "dimensions already recorded");

  RX_ASSERT(m_flags & k_type, "Type not recorded");
  RX_ASSERT(m_flags & k_format, "format not recorded");
  RX_ASSERT(m_flags & k_levels, "levels not recorded");

  if (is_compressed_format()) {
    RX_ASSERT(_dimensions.w >= 4 && _dimensions.h >= 4,
      "too small for compression");
  }

  m_dimensions = _dimensions;
  m_flags |= k_dimensions;

  DimensionType dimensions{m_dimensions};
  Size offset{0};
  const auto bpp{bits_per_pixel(m_format)};
  for (Size i{0}; i < m_levels; i++) {
    const auto size{static_cast<Size>(dimensions.area() * bpp / 8)};
    m_level_info.push_back({offset, size, dimensions});
    offset += size;
    dimensions = dimensions.map([](Size _dim) {
      return Algorithm::max(_dim / 2, 1_z);
    });
  }

  if (m_type != Type::k_attachment) {
    m_data.resize(offset, Utility::UninitializedTag{});
    update_resource_usage(m_data.size());
  }
}

void Texture2D::record_wrap(const WrapOptions& _wrap) {
  RX_ASSERT(!(m_flags & k_wrap), "wrap already recorded");

  m_wrap = _wrap;
  m_flags |= k_wrap;
}

void Texture2D::record_edit(Size _level, const DimensionType& _offset,
                            const DimensionType& _dimensions)
{
  RX_ASSERT(is_level_in_range(_level), "mipmap level out of bounds");
  m_edits.emplace_back(_level, _offset, _dimensions);
}

Size Texture2D::bytes_for_edits() const {
  Size bytes = 0;
  m_edits.each_fwd([&](const EditType& _edit) { bytes += _edit.size.area(); });
  return bytes * bits_per_pixel(format()) / 8;
}

void Texture2D::optimize_edits() {
  coalesce_edits(m_edits);
}

// Texture3D
Texture3D::Texture3D(Context* _frontend)
  : Texture{_frontend, Resource::Type::k_texture3D}
  , m_level_info{_frontend->allocator()}
  , m_edits{_frontend->allocator()}
{
}

Texture3D::~Texture3D() {
}

void Texture3D::write(const Byte* _data, Size _level) {
  RX_ASSERT(_data, "_data is null");
  RX_ASSERT(is_level_in_range(_level), "mipmap level out of bounds");
  validate();

  const auto& info{info_for_level(_level)};
  memcpy(m_data.data() + info.offset, _data, info.size);
}

Byte* Texture3D::map(Size _level) {
  RX_ASSERT(is_level_in_range(_level), "mipmap level out of bounds");
  validate();

  const auto& info{info_for_level(_level)};
  return m_data.data() + info.offset;
}

void Texture3D::record_dimensions(const Math::Vec3z& _dimensions) {
  RX_ASSERT(!(m_flags & k_dimensions), "dimensions already recorded");

  RX_ASSERT(m_flags & k_type, "Type not recorded");
  RX_ASSERT(m_flags & k_format, "format not recorded");
  RX_ASSERT(m_flags & k_levels, "levels not recorded");

  RX_ASSERT(!is_compressed_format(), "3D textures cannot be compressed");

  m_dimensions = _dimensions;
  m_flags |= k_dimensions;

  DimensionType dimensions{m_dimensions};
  Size offset{0};
  const auto bpp{bits_per_pixel(m_format)};
  for (Size i{0}; i < m_levels; i++) {
    const auto size{static_cast<Size>(dimensions.area() * bpp / 8)};
    m_level_info.push_back({offset, size, dimensions});
    offset += size;
    dimensions = dimensions.map([](Size _dim) {
      return Algorithm::max(_dim / 2, 1_z);
    });
  }

  if (m_type != Type::k_attachment) {
    m_data.resize(offset, Utility::UninitializedTag{});
    update_resource_usage(m_data.size());
  }
}

void Texture3D::record_wrap(const WrapOptions& _wrap) {
  RX_ASSERT(!(m_flags & k_wrap), "wrap already recorded");

  m_wrap = _wrap;
  m_flags |= k_wrap;
}

void Texture3D::record_edit(Size _level, const DimensionType& _offset,
                            const DimensionType& _dimensions)
{
  RX_ASSERT(is_level_in_range(_level), "mipmap level out of bounds");
  m_edits.emplace_back(_level, _offset, _dimensions);
}

Size Texture3D::bytes_for_edits() const {
  Size bytes = 0;
  m_edits.each_fwd([&](const EditType& _edit) { bytes += _edit.size.area(); });
  return bytes * bits_per_pixel(format()) / 8;
}

void Texture3D::optimize_edits() {
  coalesce_edits(m_edits);
}

// TextureCM
TextureCM::TextureCM(Context* _frontend)
  : Texture{_frontend, Resource::Type::k_textureCM}
  , m_level_info{_frontend->allocator()}
{
}

TextureCM::~TextureCM() {
}

void TextureCM::write(const Byte* _data, Face _face, Size _level) {
  RX_ASSERT(_data, "_data is null");
  RX_ASSERT(is_level_in_range(_level), "mipmap level out of bounds");
  validate();

  const auto& info{info_for_level(_level)};
  memcpy(map(_level, _face), _data, info.size / 6);
}

Byte* TextureCM::map(Size _level, Face _face) {
  RX_ASSERT(is_level_in_range(_level), "mipmap level out of bounds");
  validate();

  const auto& info{info_for_level(_level)};
  const auto face_size{info.size / 6};
  const auto face_offset{face_size * static_cast<Size>(_face)};
  return m_data.data() + info.offset + face_offset;
}

void TextureCM::record_dimensions(const Math::Vec2z& _dimensions) {
  RX_ASSERT(!(m_flags & k_dimensions), "dimensions already recorded");

  RX_ASSERT(m_flags & k_type, "Type not recorded");
  RX_ASSERT(m_flags & k_format, "format not recorded");
  RX_ASSERT(m_flags & k_levels, "levels not recorded");

  if (is_compressed_format()) {
    RX_ASSERT(_dimensions.w >= 4 && _dimensions.h >= 4,
      "too small for compression");
  }

  m_dimensions = _dimensions;
  m_flags |= k_dimensions;

  DimensionType dimensions{m_dimensions};
  Size offset{0};
  const auto bpp{bits_per_pixel(m_format)};
  for (Size i{0}; i < m_levels; i++) {
    const auto size{static_cast<Size>(dimensions.area() * bpp / 8 * 6)};
    m_level_info.push_back({offset, size, dimensions});
    offset += size;
    dimensions = dimensions.map([](Size _dim) {
      return Algorithm::max(_dim / 2, 1_z);
    });
  }

  if (m_type != Type::k_attachment) {
    m_data.resize(offset, Utility::UninitializedTag{});
    update_resource_usage(m_data.size());
  }
}

void TextureCM::record_wrap(const WrapOptions& _wrap) {
  RX_ASSERT(!(m_flags & k_wrap), "wrap already recorded");

  m_wrap = _wrap;
  m_flags |= k_wrap;
}

} // namespace rx::render::frontend
