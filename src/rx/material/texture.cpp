#include "rx/core/filesystem/unbuffered_file.h"

#include "rx/core/algorithm/clamp.h"

#include "rx/core/hash/djbx33a.h"
#include "rx/core/serialize/json.h"

#include "rx/math/transform.h"

#include "rx/material/texture.h"

#include "rx/texture/loader.h"

namespace Rx::Material {

RX_LOG("material/texture", logger);

Texture::Texture(Memory::Allocator& _allocator)
  : m_allocator{&_allocator}
  , m_file{allocator()}
  , m_report{allocator(), *logger}
{
}

bool Texture::load(Stream::Context& _stream) {
  if (auto contents = _stream.read_text(allocator())) {
    if (auto disown = contents->disown()) {
      if (auto json = Serialize::JSON::parse(allocator(), String{*disown})) {
        return parse(*json);
      }
    }
  }
  return false;
}

bool Texture::load(const StringView& _file_name) {
  if (auto file = Filesystem::UnbufferedFile::open(allocator(), _file_name, "r")) {
    return load(*file);
  }
  return false;
}

bool Texture::parse(const Serialize::JSON& _definition) {
  if (!_definition) {
    const auto json_error = _definition.error();
    if (json_error) {
      return m_report.error("%s", *json_error);
    } else {
      return m_report.error("empty definition");
    }
  }

  const auto& file = _definition["file"];
  const auto& type = _definition["type"];

  if (!file) {
    return m_report.error("missing 'file'");
  }

  if (!type) {
    return m_report.error("missing 'type'");
  }

  if (!parse_type(type)) {
    return false;
  }

  auto file_name = file.as_string(allocator());
  if (!file_name) {
    return false;
  }

  const auto& filter = _definition["filter"];
  const auto& mipmap_mode = _definition["mipmap_mode"];

  if (filter && !parse_filter(filter)) {
    return false;
  }

  if (mipmap_mode && !parse_mipmap_mode(mipmap_mode)) {
    return false;
  }

  m_file = Utility::move(*file_name);

  // TODO(dweiler): Inject the max dimensions from a higher level place.
  return load_texture_file({4096, 4096});
}

bool Texture::load_texture_file(const Math::Vec2z& _max_dimensions) {
  Rx::Texture::PixelFormat want_format;
  switch (m_type) {
  case Type::ALBEDO:
    want_format = Rx::Texture::PixelFormat::SRGBA_U8;
    break;
  case Type::METALNESS:
    [[fallthrough]];
  case Type::ROUGHNESS:
    want_format = Rx::Texture::PixelFormat::R_U8;
    break;
  default:
    want_format = Rx::Texture::PixelFormat::RGB_U8;
    break;
  }

  Rx::Texture::Loader loader{allocator()};
  if (!loader.load(m_file, want_format, _max_dimensions)) {
    return m_report.error("failed to load file \"%s\"", m_file);
  }

  auto data = Utility::move(loader.data());

  m_bitmap.format = loader.format();
  m_bitmap.dimensions = loader.dimensions();
  m_bitmap.hash = Hash::djbx33a(data.data(), data.size());
  m_bitmap.data = Utility::move(data);

  return true;
}

bool Texture::parse_type(const Serialize::JSON& _type) {
  if (!_type.is_string()) {
    return m_report.error("expected String");
  }

  static constexpr struct {
    const char* name;
    Type type;
  } MATCHES[] = {
    { "albedo",    Type::ALBEDO    },
    { "normal",    Type::NORMAL    },
    { "metalness", Type::METALNESS },
    { "roughness", Type::ROUGHNESS },
    { "occlusion", Type::OCCLUSION },
    { "emissive",  Type::EMISSIVE  },
    { "custom",    Type::CUSTOM    }
  };

  const auto& type_string = _type.as_string(allocator());
  if (!type_string) {
    return false;
  }

  const auto find_match = [this](const String& _type) -> Optional<Type> {
    for (const auto& match : MATCHES) {
      if (match.name == _type) {
        return match.type;
      }
    }
    return m_report.error("unknown type '%s'", _type);
  };

  const auto& type = find_match(*type_string);
  if (!type) {
    return false;
  }
 
  m_type = *type;

  return true;
}

bool Texture::parse_filter(const Serialize::JSON& _filter) {
  if (!_filter.is_object()) {
    return m_report.error("expected Object for 'filter'");
  }

  const auto& min = _filter["min"];
  const auto& mag = _filter["mag"];

  if (!min) {
    return m_report.error("missing 'min'");
  }

  if (!mag) {
    return m_report.error("missing 'mag'");
  }

  if (!min.is_string()) {
    return m_report.error("expected String for 'min'");
  }

  if (!mag.is_string()) {
    return m_report.error("expected String for 'mag'");
  }

  static constexpr const struct {
    const char* match;
    Filter filter;
  } MATCHES[] = {
    { "linear",  Filter::LINEAR  },
    { "nearest", Filter::NEAREST }
  };

  const auto& min_string = min.as_string(allocator());
  const auto& mag_string = mag.as_string(allocator());
  if (!min_string || !mag_string) {
    return false;
  }

  const auto find_match = [this](const String& _type) -> Optional<Filter> {
    for (const auto& match : MATCHES) {
      if (_type == match.match) {
        return match.filter;
      }
    }
    return m_report.error("invalid filter '%s'", _type);
  };

  const auto& min_filter = find_match(*min_string);
  const auto& mag_filter = find_match(*mag_string);
  if (!min_filter || !mag_filter) {
    return false;
  }

  m_min_filter = *min_filter;
  m_mag_filter = *mag_filter;

  return true;
}

bool Texture::parse_mipmap_mode(const Serialize::JSON& _mipmap_mode) {
  if (!_mipmap_mode.is_string()) {
    return m_report.error("expected String for 'mipmap_mode'");
  }

  const auto& mipmap_mode_string = _mipmap_mode.as_string(allocator());
  if (!mipmap_mode_string) {
    return false;
  }

  static constexpr const struct {
    const char* match;
    MipmapMode mipmap_mode;
  } MATCHES[] = {
    { "none",    MipmapMode::NONE    },
    { "nearest", MipmapMode::NEAREST },
    { "linear",  MipmapMode::LINEAR  }
  };

  const auto find_match = [this](const String& _type) -> Optional<MipmapMode> {
    for (const auto& match : MATCHES) {
      if (_type == match.match) {
        return match.mipmap_mode;
      }
    }
    return m_report.error("invalid mipamp mode '%s'", _type);
  };

  const auto& mipmap_mode = find_match(*mipmap_mode_string);
  if (!mipmap_mode) {
    return false;
  }

  m_mipmap_mode = *mipmap_mode;

  return true;
}

bool Texture::parse_address_mode(const Serialize::JSON& _address_mode) {
  if (!_address_mode.is_array_of(Serialize::JSON::Type::STRING, 2)) {
    return m_report.error("expected Array[String, 2]");
  }

  static constexpr const struct {
    const char* match;
    AddressMode type;
  } MATCHES[] = {
    { "clamp_to_edge",   AddressMode::CLAMP_TO_EDGE   },
    { "mirrored_repeat", AddressMode::MIRRORED_REPEAT },
    { "repeat",          AddressMode::REPEAT          }
  };

  const auto find_match = [this](const String& _type) -> Optional<AddressMode> {
    for (const auto& match : MATCHES) {
      if (_type == match.match) {
        return match.type;
      }
    }
    return m_report.error("invalid address mode '%s'", _type);
  };

  const auto& address_mode_u_string = _address_mode[0_z].as_string(allocator());
  const auto& address_mode_v_string = _address_mode[1_z].as_string(allocator());
  if (!address_mode_u_string || !address_mode_v_string) {
    return false;
  }

  const auto& address_mode_u = find_match(*address_mode_u_string);
  const auto& address_mode_v = find_match(*address_mode_v_string);
  if (!address_mode_u || !address_mode_v) {
    return false;
  }

  m_address_mode_u = *address_mode_u;
  m_address_mode_v = *address_mode_v;

  return true;
}

} // namespace Rx::Material
