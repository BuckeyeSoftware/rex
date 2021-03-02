#include <string.h> // memcpy

#include "rx/render/frontend/texture.h"
#include "rx/render/frontend/context.h"

#include "rx/core/algorithm/quick_sort.h"

#include "rx/core/math/log2.h"

namespace Rx::Render::Frontend {

template<typename T>
struct ChainInfo {
  RX_MARK_NO_COPY(ChainInfo);

  constexpr ChainInfo()
    : size{0}
  {
  }

  ChainInfo(ChainInfo&& chain_info_)
    : levels{Utility::move(chain_info_.levels)}
    , size{Utility::exchange(chain_info_.size, 0)}
  {
  }

  Vector<Texture::LevelInfo<T>> levels;
  Size size;
};

template<Resource::Type KIND, typename T>
static Optional<ChainInfo<T>>
calculate_levels(Size _bits_per_pixel, Size _levels, const T& _dimensions) {
  ChainInfo<T> result;

  if (!result.levels.reserve(_levels)) {
    return nullopt;
  }

  auto dimensions = _dimensions;
  Size offset = 0;
  for (Size i = 0; i < _levels; i++) {
    Size size = 0;
    if constexpr (KIND == Texture::Resource::Type::TEXTURE1D) {
      size = (dimensions * _bits_per_pixel) / 8;
    } else if constexpr (KIND == Texture::Resource::Type::TEXTURECM) {
      size = (dimensions.area() * _bits_per_pixel * 6) / 8;
    } else {
      size = (dimensions.area() * _bits_per_pixel) / 8;
    }

    if (!result.levels.emplace_back(offset, size, dimensions)) {
      return nullopt;
    }

    offset += size;

    if constexpr (KIND == Texture::Resource::Type::TEXTURE1D) {
      dimensions = Algorithm::max(dimensions / 2, 1_z);
    } else {
      dimensions = dimensions.map([](Size _dim) {
        return Algorithm::max(_dim / 2, 1_z);
      });
    }
  }

  result.size = offset;

  return result;
}

Texture::Texture(Context* _frontend, Resource::Type _type)
  : Resource{_frontend, _type}
  , m_data{_frontend->allocator()}
  , m_flags{0}
{
}

void Texture::record_format(DataFormat _format) {
  RX_ASSERT(!(m_flags & FORMAT), "format already recorded");
  m_format = _format;
  m_flags |= FORMAT;
}

void Texture::record_type(Type _type) {
  RX_ASSERT(!(m_flags & TYPE), "type already recorded");
  m_type = _type;
  m_flags |= TYPE;
}

void Texture::record_filter(const FilterOptions& _options) {
  RX_ASSERT(!(m_flags & FILTER), "filter already recorded");
  m_filter = _options;
  m_flags |= FILTER;
}

void Texture::record_levels(Size _levels) {
  RX_ASSERT(!(m_flags & LEVELS), "levels already recorded");
  RX_ASSERT(_levels, "_levels must be at least 1");

  m_levels = _levels;
  m_flags |= LEVELS;
}

void Texture::record_border(const Math::Vec4f& _border) {
  RX_ASSERT(!(m_flags & BORDER), "border already recorded");
  RX_ASSERT(m_flags & WRAP, "wrap not recorded");

  m_border = _border;
  m_flags |= BORDER;
}

void Texture::validate() const {
  RX_ASSERT(m_flags & FORMAT, "format not recorded");
  RX_ASSERT(m_flags & TYPE, "type not recorded");
  RX_ASSERT(m_flags & FILTER, "filter not recorded");
  RX_ASSERT(m_flags & WRAP, "wrap not recorded");
  RX_ASSERT(m_flags & DIMENSIONS, "dimensions not recorded");
  RX_ASSERT(m_flags & LEVELS, "levels not recorded");

  if (m_filter.mipmaps) {
    RX_ASSERT(m_levels > 1, "no levels specified for mipmaps");
  }

  if (m_flags & SWAPCHAIN) {
    RX_ASSERT(!m_filter.mipmaps, "swapchain cannot have mipmaps");
    RX_ASSERT(!m_filter.bilinear, "swapchain cannot have bilinear filtering");
    RX_ASSERT(!m_filter.trilinear, "swapchain cannot have trilinear filtering");
    RX_ASSERT(m_levels == 1, "swapchain cannot have levels");
  }
}

// Texture1D
Texture1D::Texture1D(Context* _frontend)
  : Texture{_frontend, Resource::Type::TEXTURE1D}
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

bool Texture1D::record_dimensions(const DimensionType& _dimensions) {
  RX_ASSERT(!(m_flags & DIMENSIONS), "dimensions already recorded");

  RX_ASSERT(m_flags & TYPE, "type not recorded");
  RX_ASSERT(m_flags & FORMAT, "format not recorded");
  RX_ASSERT(m_flags & LEVELS, "levels not recorded");

  RX_ASSERT(!is_compressed_format(), "1D textures cannot be compressed");

  auto levels = calculate_levels<Resource::Type::TEXTURE1D>(bits_per_pixel(), m_levels, _dimensions);
  if (!levels) {
    return false;
  }

  if (m_type != Type::ATTACHMENT && !m_data.resize(levels->size)) {
    return false;
  }

  update_resource_usage(levels->size);

  m_dimensions = _dimensions;
  m_flags |= DIMENSIONS;
  m_level_info = Utility::move(levels->levels);

  return true;
}

void Texture1D::record_wrap(const WrapOptions& _wrap) {
  RX_ASSERT(!(m_flags & WRAP), "wrap already recorded");

  m_wrap = _wrap;
  m_flags |= WRAP;
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
  return bytes * bits_per_pixel() / 8;
}

// Texture2D
Texture2D::Texture2D(Context* _frontend)
  : Texture{_frontend, Resource::Type::TEXTURE2D}
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

bool Texture2D::record_dimensions(const Math::Vec2z& _dimensions) {
  RX_ASSERT(!(m_flags & DIMENSIONS), "dimensions already recorded");

  RX_ASSERT(m_flags & TYPE, "type not recorded");
  RX_ASSERT(m_flags & FORMAT, "format not recorded");
  RX_ASSERT(m_flags & LEVELS, "levels not recorded");

  if (is_compressed_format()) {
    RX_ASSERT(_dimensions.w >= 4 && _dimensions.h >= 4,
      "too small for compression");
  }

  auto levels = calculate_levels<Resource::Type::TEXTURE2D>(bits_per_pixel(), m_levels, _dimensions);
  if (!levels) {
    return false;
  }

  if (m_type != Type::ATTACHMENT && !m_data.resize(levels->size)) {
    return false;
  }

  update_resource_usage(levels->size);

  m_dimensions = _dimensions;
  m_flags |= DIMENSIONS;
  m_level_info = Utility::move(levels->levels);

  return true;
}

void Texture2D::record_wrap(const WrapOptions& _wrap) {
  RX_ASSERT(!(m_flags & WRAP), "wrap already recorded");

  m_wrap = _wrap;
  m_flags |= WRAP;
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
  return bytes * bits_per_pixel() / 8;
}

// Texture3D
Texture3D::Texture3D(Context* _frontend)
  : Texture{_frontend, Resource::Type::TEXTURE3D}
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

bool Texture3D::record_dimensions(const Math::Vec3z& _dimensions) {
  RX_ASSERT(!(m_flags & DIMENSIONS), "dimensions already recorded");

  RX_ASSERT(m_flags & TYPE, "type not recorded");
  RX_ASSERT(m_flags & FORMAT, "format not recorded");
  RX_ASSERT(m_flags & LEVELS, "levels not recorded");

  RX_ASSERT(!is_compressed_format(), "3D textures cannot be compressed");

  auto levels = calculate_levels<Resource::Type::TEXTURE3D>(bits_per_pixel(), m_levels, _dimensions);
  if (!levels) {
    return false;
  }

  if (m_type != Type::ATTACHMENT && !m_data.resize(levels->size)) {
    return false;
  }

  update_resource_usage(levels->size);

  m_dimensions = _dimensions;
  m_flags |= DIMENSIONS;
  m_level_info = Utility::move(levels->levels);

  return true;
}

void Texture3D::record_wrap(const WrapOptions& _wrap) {
  RX_ASSERT(!(m_flags & WRAP), "wrap already recorded");

  m_wrap = _wrap;
  m_flags |= WRAP;
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
  return bytes * bits_per_pixel() / 8;
}

// TextureCM
TextureCM::TextureCM(Context* _frontend)
  : Texture{_frontend, Resource::Type::TEXTURECM}
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

bool TextureCM::record_dimensions(const Math::Vec2z& _dimensions) {
  RX_ASSERT(!(m_flags & DIMENSIONS), "dimensions already recorded");

  RX_ASSERT(m_flags & TYPE, "type not recorded");
  RX_ASSERT(m_flags & FORMAT, "format not recorded");
  RX_ASSERT(m_flags & LEVELS, "levels not recorded");

  if (is_compressed_format()) {
    RX_ASSERT(_dimensions.w >= 4 && _dimensions.h >= 4,
      "too small for compression");
  }

  auto levels = calculate_levels<Resource::Type::TEXTURECM>(bits_per_pixel(), m_levels, _dimensions);
  if (!levels) {
    return false;
  }

  if (m_type != Type::ATTACHMENT && !m_data.resize(levels->size)) {
    return false;
  }

  update_resource_usage(levels->size);

  m_dimensions = _dimensions;
  m_flags |= DIMENSIONS;
  m_level_info = Utility::move(levels->levels);

  return true;
}

void TextureCM::record_wrap(const WrapOptions& _wrap) {
  RX_ASSERT(!(m_flags & WRAP), "wrap already recorded");

  m_wrap = _wrap;
  m_flags |= WRAP;
}

} // namespace Rx::Render::Frontend
