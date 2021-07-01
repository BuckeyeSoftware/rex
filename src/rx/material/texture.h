#ifndef RX_MATERIAL_TEXTURE_H
#define RX_MATERIAL_TEXTURE_H
#include "rx/core/array.h"
#include "rx/core/log.h"
#include "rx/core/optional.h"
#include "rx/core/report.h"

#include "rx/math/transform.h"

#include "rx/texture/chain.h"

namespace Rx { struct JSON; }
namespace Rx::Stream { struct UntrackedStream; }

namespace Rx::Material {

struct Texture {
  RX_MARK_NO_COPY(Texture);

  struct Filter {
    bool bilinear;
    bool trilinear;
    bool mipmaps;
  };

  enum class WrapType : Byte {
    CLAMP_TO_EDGE,
    CLAMP_TO_BORDER,
    MIRRORED_REPEAT,
    REPEAT,
    MIRROR_CLAMP_TO_EDGE
  };

  enum class Type : Byte {
    ALBEDO,
    NORMAL,
    METALNESS,
    ROUGHNESS,
    OCCLUSION,
    EMISSIVE,
    CUSTOM
  };

  using Wrap = Math::Vec2<WrapType>;

  struct Bitmap {
    LinearBuffer data;
    Array<Byte[16]> hash;
    Rx::Texture::PixelFormat format;
    Math::Vec2z dimensions;
  };

  Texture(Memory::Allocator& _allocator);
  Texture(Texture&& texture_);

  Texture& operator=(Texture&& texture_);

  [[nodiscard]] bool load(Stream::UntrackedStream& _stream);
  [[nodiscard]] bool load(const StringView& _file_name);

  [[nodiscard]] bool parse(const JSON& _definition);

  const Filter& filter() const &;
  const Wrap& wrap() const &;
  Type type() const;
  const String& file() const &;
  const Optional<Math::Vec4f>& border() const;

  const Bitmap& bitmap() const;

  constexpr Memory::Allocator& allocator() const;

private:
  bool load_texture_file(const Math::Vec2z& _max_dimensions);

  bool parse_type(const JSON& _type);
  bool parse_filter(const JSON& _filter, bool& _mipmaps);
  bool parse_wrap(const JSON& _wrap);
  bool parse_border(const JSON& _border);

  Memory::Allocator* m_allocator;
  Bitmap m_bitmap;
  Filter m_filter;
  Wrap m_wrap;
  Type m_type;
  String m_file;
  Optional<Math::Vec4f> m_border;
  Report m_report;
};

inline Texture::Texture(Texture&& texture_)
  : m_allocator{texture_.m_allocator}
  , m_bitmap{Utility::move(texture_.m_bitmap)}
  , m_filter{texture_.m_filter}
  , m_wrap{texture_.m_wrap}
  , m_type{texture_.m_type}
  , m_file{Utility::move(texture_.m_file)}
  , m_border{Utility::move(texture_.m_border)}
  , m_report{Utility::move(texture_.m_report)}
{
}

inline Texture& Texture::operator=(Texture&& texture_) {
  if (&texture_ != this) {
    m_allocator = texture_.m_allocator;
    m_bitmap = Utility::move(texture_.m_bitmap);
    m_filter = texture_.m_filter;
    m_wrap = texture_.m_wrap;
    m_type = texture_.m_type;
    m_file = Utility::move(texture_.m_file);
    m_border = Utility::move(texture_.m_border);
    m_report = Utility::move(texture_.m_report);
  }
  return *this;
}

inline const Texture::Filter& Texture::filter() const & {
  return m_filter;
}

inline const Texture::Wrap& Texture::wrap() const & {
  return m_wrap;
}

inline Texture::Type Texture::type() const {
  return m_type;
}

inline const String& Texture::file() const & {
  return m_file;
}

inline const Optional<Math::Vec4f>& Texture::border() const {
  return m_border;
}

inline const Texture::Bitmap& Texture::bitmap() const {
  return m_bitmap;
}

RX_HINT_FORCE_INLINE constexpr Memory::Allocator& Texture::allocator() const {
  return *m_allocator;
}

} // namespace Rx::Material

#endif // RX_MATERIAL_TEXTURE_H
