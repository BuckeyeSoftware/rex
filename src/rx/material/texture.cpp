#include "rx/core/filesystem/file.h"
#include "rx/core/algorithm/clamp.h"
#include "rx/core/math/log2.h"
#include "rx/core/json.h"

#include "rx/math/transform.h"

#include "rx/material/texture.h"

#include "rx/texture/loader.h"
#include "rx/texture/chain.h"

namespace Rx::Material {

RX_LOG("material/texture", logger);

bool Texture::load(Stream* _stream) {
  if (auto contents = read_text_stream(allocator(), _stream)) {
    if (auto disown = contents->disown()) {
      return parse({*disown});
    }
  }
  return false;
}

bool Texture::load(const String& _file_name) {
  if (Filesystem::File file{_file_name, "rb"}) {
    return load(&file);
  }
  return false;
}

bool Texture::parse(const JSON& _definition) {
  if (!_definition) {
    const auto json_error{_definition.error()};
    if (json_error) {
      return error("%s", *json_error);
    } else {
      return error("empty definition");
    }
  }

  const auto& file{_definition["file"]};
  const auto& type{_definition["type"]};
  const auto& filter{_definition["filter"]};
  const auto& wrap{_definition["wrap"]};
  const auto& mipmaps{_definition["mipmaps"]};
  const auto& border{_definition["border"]};

  if (!file) {
    return error("missing 'file'");
  }

  if (!type) {
    return error("missing 'type'");
  }

  if (!filter) {
    return error("missing 'filter'");
  }

  if (!file.is_string()) {
    return error("expected String for 'file'");
  }

  if (mipmaps && !mipmaps.is_boolean()) {
    return error("expected Boolean for 'mipmaps'");
  }

  if (border && !parse_border(border)) {
    return false;
  }

  if (!parse_type(type)) {
    return false;
  }

  bool has_mipmaps{mipmaps ? mipmaps.as_boolean() : false};
  if (!parse_filter(filter, has_mipmaps)) {
    return false;
  }

  if (!parse_wrap(wrap)) {
    return false;
  }

  if (m_wrap.is_any(WrapType::k_clamp_to_border) && !m_border) {
    return error("missing 'border' for \"clamp_to_border\"");
  }

  m_file = file.as_string();

  // TODO(dweiler): Inject the max dimensions from a higher level place.
  return load_texture_file({4096, 4096});
}

bool Texture::load_texture_file(const Math::Vec2z& _max_dimensions) {
  Rx::Texture::PixelFormat want_format;
  if (m_type == "albedo") {
    want_format = Rx::Texture::PixelFormat::RGBA_U8;
  } else if (m_type == "metalness" || m_type == "roughness") {
    want_format = Rx::Texture::PixelFormat::R_U8;
  } else {
    want_format = Rx::Texture::PixelFormat::RGB_U8;
  }

  Rx::Texture::Loader loader{allocator()};
  if (!loader.load(m_file, want_format, _max_dimensions)) {
    return false;
  }

  return m_chain.generate(Utility::move(loader), false, m_filter.mipmaps);
}

bool Texture::parse_type(const JSON& _type) {
  if (!_type.is_string()) {
    return error("expected String");
  }

  static constexpr const char* k_matches[]{
    "albedo",
    "normal",
    "metalness",
    "roughness",
    "occlusion",
    "emission"
  };

  for (const auto& match : k_matches) {
    if (match == _type.as_string()) {
      m_type = match;
      return true;
    }
  }

  return error("unknown Type '%s'", _type.as_string());
}

bool Texture::parse_filter(const JSON& _filter, bool& _mipmaps) {
  if (!_filter.is_string()) {
    return error("expected String");
  }

  static constexpr const char* k_matches[] = {
    "bilinear",
    "trilinear",
    "nearest"
  };

  const auto& filter_string = _filter.as_string();
  for (const auto& match : k_matches) {
    if (filter_string == match) {
      const bool trilinear = *match == 't';
      const bool bilinear = trilinear || *match == 'b'; // trilinear is an extension of bilinear
      const bool mipmaps = trilinear || _mipmaps; // trilinear needs mipmaps

      m_filter.trilinear = trilinear;
      m_filter.bilinear = bilinear;
      m_filter.mipmaps = mipmaps;

      _mipmaps = mipmaps;

      return true;
    }
  }

  return error("unknown filter '%s'", filter_string);
}

bool Texture::parse_wrap(const JSON& _wrap) {
  if (!_wrap.is_array_of(JSON::Type::k_string) || _wrap.size() != 2) {
    return error("expected Array[String, 2]");
  }

  static constexpr const struct {
    const char* match;
    WrapType type;
  } k_matches[]{
    { "clamp_to_edge",        WrapType::k_clamp_to_edge        },
    { "clamp_to_border",      WrapType::k_clamp_to_border      },
    { "mirrored_repeat",      WrapType::k_mirrored_repeat      },
    { "repeat",               WrapType::k_repeat               },
    { "mirror_clamp_to_edge", WrapType::k_mirror_clamp_to_edge }
  };

  const auto parse = [this](const String& _type) -> Optional<WrapType> {
    for (const auto& match : k_matches) {
      if (_type == match.match) {
        return match.type;
      }
    }
    error("invalid wrap Type '%s'", _type);
    return nullopt;
  };

  const auto& s_wrap = parse(_wrap[0_z].as_string());
  const auto& t_wrap = parse(_wrap[1_z].as_string());

  if (s_wrap && t_wrap) {
    m_wrap.s = *s_wrap;
    m_wrap.t = *t_wrap;
    return true;
  }

  return false;
}

bool Texture::parse_border(const JSON& _border) {
  if (!_border.is_array_of(JSON::Type::k_number) || _border.size() != 4) {
    return error("expected Array[Number, 4]");
  }

  m_border = {
    Algorithm::clamp(_border[0_z].as_float(), 0.0f, 1.0f),
    Algorithm::clamp(_border[1_z].as_float(), 0.0f, 1.0f),
    Algorithm::clamp(_border[2_z].as_float(), 0.0f, 1.0f),
    Algorithm::clamp(_border[3_z].as_float(), 0.0f, 1.0f)
  };

  return true;
}

void Texture::write_log(Log::Level _level, String&& message_) const {
  if (m_type.is_empty()) {
    logger->write(_level, "%s", Utility::move(message_));
  } else {
    logger->write(_level, "%s: %s", m_type, Utility::move(message_));
  }
}

} // namespace Rx::Material
