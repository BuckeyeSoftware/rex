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
    const auto json_error{_definition.error()};
    if (json_error) {
      return m_report.error("%s", *json_error);
    } else {
      return m_report.error("empty definition");
    }
  }

  const auto& file{_definition["file"]};
  const auto& type{_definition["type"]};
  const auto& filter{_definition["filter"]};
  const auto& address_mode{_definition["wrap"]};
  const auto& mipmaps{_definition["mipmaps"]};

  if (!file) {
    return m_report.error("missing 'file'");
  }

  if (!type) {
    return m_report.error("missing 'type'");
  }

  if (!filter) {
    return m_report.error("missing 'filter'");
  }

  if (!file.is_string()) {
    return m_report.error("expected String for 'file'");
  }

  auto file_name = file.as_string(allocator());
  if (!file_name) {
    return false;
  }

  if (mipmaps && !mipmaps.is_boolean()) {
    return m_report.error("expected Boolean for 'mipmaps'");
  }

  if (!parse_type(type)) {
    return false;
  }

  bool has_mipmaps{mipmaps ? mipmaps.as_boolean() : false};
  if (!parse_filter(filter, has_mipmaps)) {
    return false;
  }

  if (!parse_address_mode(address_mode)) {
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

  auto type = _type.as_string(allocator());
  if (!type) {
    return false;
  }

  for (const auto& match : MATCHES) {
    if (match.name != *type) {
      continue;
    }
    m_type = match.type;
    return true;
  }

  return m_report.error("unknown type '%s'", *type);
}

bool Texture::parse_filter(const Serialize::JSON& _filter, bool& _mipmaps) {
  if (!_filter.is_string()) {
    return m_report.error("expected String");
  }

  static constexpr const char* MATCHES[] = {
    "bilinear",
    "trilinear",
    "nearest"
  };

  auto filter = _filter.as_string(allocator());
  if (!filter) {
    return false;
  }

  for (const auto& match : MATCHES) {
    if (*filter != match) {
      continue;
    }

    const bool trilinear = *match == 't';
    const bool bilinear = trilinear || *match == 'b'; // trilinear is an extension of bilinear
    const bool mipmaps = trilinear || _mipmaps; // trilinear needs mipmaps

    m_filter.trilinear = trilinear;
    m_filter.bilinear = bilinear;
    m_filter.mipmaps = mipmaps;

    _mipmaps = mipmaps;

    return true;
  }

  return m_report.error("unknown filter '%s'", *filter);
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

  const auto parse = [this](const String& _type) -> Optional<AddressMode> {
    for (const auto& match : MATCHES) {
      if (_type == match.match) {
        return match.type;
      }
    }
    return m_report.error("invalid address mode '%s'", _type);
  };

  auto s_string = _address_mode[0_z].as_string(allocator());
  auto t_string = _address_mode[1_z].as_string(allocator());
  if (!s_string || !t_string) {
    return false;
  }

  const auto u_address_mode = parse(*s_string);
  const auto v_address_mode = parse(*t_string);
  if (!u_address_mode || !v_address_mode) {
    return false;
  }

  m_address_mode_u = *u_address_mode;
  m_address_mode_v = *v_address_mode;

  return true;
}

} // namespace Rx::Material
