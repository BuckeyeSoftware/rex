#ifndef RX_MATERIAL_TEXTURE_H
#define RX_MATERIAL_TEXTURE_H
#include "rx/core/array.h"
#include "rx/core/log.h"
#include "rx/core/optional.h"
#include "rx/core/report.h"

#include "rx/math/transform.h"

#include "rx/texture/chain.h"

namespace Rx::Serialize { struct JSON; }
namespace Rx::Stream { struct Context; }

namespace Rx::Material {

struct Texture {
  RX_MARK_NO_COPY(Texture);

  enum class Filter {
    NEAREST, ///< Nearest filtering within.
    LINEAR   ///< Linear filtering between (Bilinear.)
  };

  /// The type of filtering to use on mipmaps.
  enum class MipmapMode {
    NONE,    ///< No mipmaps.
    NEAREST, ///< Nearest filtering within a mip level.
    LINEAR   ///< Linear filtering between mip level (Trilinear.)
  };

  enum class AddressMode : Byte {
    REPEAT,
    MIRRORED_REPEAT,
    CLAMP_TO_EDGE
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

  struct Bitmap {
    LinearBuffer data;
    Array<Byte[16]> hash;
    Rx::Texture::PixelFormat format;
    Math::Vec2z dimensions;
  };

  Texture(Memory::Allocator& _allocator);
  Texture(Texture&& texture_);

  Texture& operator=(Texture&& texture_);

  [[nodiscard]] bool load(Stream::Context& _stream);
  [[nodiscard]] bool load(const StringView& _file_name);

  [[nodiscard]] bool parse(const Serialize::JSON& _definition);

  const Filter& min_filter() const;
  const Filter& mag_filter() const;
  const MipmapMode& mipmap_mode() const;
  const AddressMode& address_mode_u() const;
  const AddressMode& address_mode_v() const;
  Type type() const;
  const String& file() const &;

  const Bitmap& bitmap() const;

  constexpr Memory::Allocator& allocator() const;

private:
  bool load_texture_file(const Math::Vec2z& _max_dimensions);

  [[nodiscard]] bool parse_type(const Serialize::JSON& _type);
  [[nodiscard]] bool parse_filter(const Serialize::JSON& _filter);
  [[nodiscard]] bool parse_mipmap_mode(const Serialize::JSON& _mipmap_mode);
  [[nodiscard]] bool parse_address_mode(const Serialize::JSON& _address_mode);

  Memory::Allocator* m_allocator;
  Bitmap m_bitmap;
  Filter m_min_filter;
  Filter m_mag_filter;
  MipmapMode m_mipmap_mode;
  AddressMode m_address_mode_u;
  AddressMode m_address_mode_v;
  Type m_type;
  String m_file;
  Report m_report;
};

inline Texture::Texture(Texture&& texture_)
  : m_allocator{texture_.m_allocator}
  , m_bitmap{Utility::move(texture_.m_bitmap)}
  , m_min_filter{texture_.m_min_filter}
  , m_mag_filter{texture_.m_mag_filter}
  , m_mipmap_mode{texture_.m_mipmap_mode}
  , m_address_mode_u{texture_.m_address_mode_u}
  , m_address_mode_v{texture_.m_address_mode_v}
  , m_type{texture_.m_type}
  , m_file{Utility::move(texture_.m_file)}
  , m_report{Utility::move(texture_.m_report)}
{
}

inline Texture& Texture::operator=(Texture&& texture_) {
  if (&texture_ != this) {
    m_allocator = texture_.m_allocator;
    m_bitmap = Utility::move(texture_.m_bitmap);
    m_min_filter = texture_.m_min_filter;
    m_mag_filter = texture_.m_mag_filter;
    m_mipmap_mode = texture_.m_mipmap_mode;
    m_address_mode_u = texture_.m_address_mode_u;
    m_address_mode_v = texture_.m_address_mode_v;
    m_type = texture_.m_type;
    m_file = Utility::move(texture_.m_file);
    m_report = Utility::move(texture_.m_report);
  }
  return *this;
}

inline const Texture::Filter& Texture::min_filter() const {
  return m_min_filter;
}

inline const Texture::Filter& Texture::mag_filter() const {
  return m_mag_filter;
}

inline const Texture::MipmapMode& Texture::mipmap_mode() const {
  return m_mipmap_mode;
}

inline const Texture::AddressMode& Texture::address_mode_u() const {
  return m_address_mode_u;
}

inline const Texture::AddressMode& Texture::address_mode_v() const {
  return m_address_mode_v;
}

inline Texture::Type Texture::type() const {
  return m_type;
}

inline const String& Texture::file() const & {
  return m_file;
}

inline const Texture::Bitmap& Texture::bitmap() const {
  return m_bitmap;
}

RX_HINT_FORCE_INLINE constexpr Memory::Allocator& Texture::allocator() const {
  return *m_allocator;
}

} // namespace Rx::Material

#endif // RX_MATERIAL_TEXTURE_H
