#include "rx/core/filesystem/file.h"
#include "rx/core/json.h"
#include "rx/core/math/log2.h"

#include "rx/material/texture.h"

#include "rx/texture/loader.h"
#include "rx/texture/chain.h"

namespace rx::material {

RX_LOG("material/texture", logger);

bool texture::load(const string& _file_name) {
  auto contents{filesystem::read_binary_file(m_allocator, _file_name)};
  if (!contents) {
    return false;
  }
  return parse({contents->disown()});
}

bool texture::parse(const json& _definition) {
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

  if (!parse_type(type)) {
    return false;
  }

  if (mipmaps && !mipmaps.is_boolean()) {
    return error("expected Boolean for 'mipmaps'");
  }

  if (!parse_filter(filter, mipmaps.as_boolean())) {
    return false;
  }

  if (!parse_wrap(wrap)) {
    return false;
  }

  rx::texture::loader loader{m_allocator};
  if (!loader.load(file.as_string())) {
    return false;
  }

  m_chain.generate(utility::move(loader), false, mipmaps.as_boolean());

  return true;
}

bool texture::parse_type(const json& _type) {
  if (!_type.is_string()) {
    return error("expected String");
  }

  static constexpr const char* k_matches[]{
    "diffuse",
    "normal",
    "metalness",
    "roughness"
  };

  for (const auto& match : k_matches) {
    if (match == _type.as_string()) {
      m_type = match;
      return true;
    }
  }

  return error("unknown type '%s'", _type.as_string());
}

bool texture::parse_filter(const json& _filter, bool _mipmaps) {
  if (!_filter.is_string()) {
    return error("expected String");
  }

  static constexpr const char* k_matches[]{
    "bilinear",
    "trilinear",
    "nearest"
  };

  const auto& filter_string{_filter.as_string()};
  for (const auto& match : k_matches) {
    if (filter_string == match) {
      const bool trilinear{*match == 't'};
      const bool bilinear{trilinear || *match == 'b'}; // trilinear is an extension of bilinear
      const bool mipmaps{trilinear || _mipmaps}; // trilinear needs mipmaps

      m_filter.trilinear = trilinear;
      m_filter.bilinear = bilinear;
      m_filter.mipmaps = mipmaps;

      return true;
    }
  }

  return error("unknown filter '%s'", filter_string);
}

bool texture::parse_wrap(const json& _wrap) {
  if (!_wrap.is_array_of(json::type::k_string) || _wrap.size() != 2) {
    return error("expected Array[String, 2]");
  }

  static constexpr const struct {
    const char* match;
    wrap_type type;
  } k_matches[]{
    { "clamp_to_edge",        wrap_type::k_clamp_to_edge        },
    { "clamp_to_border",      wrap_type::k_clamp_to_border      },
    { "mirrored_repeat",      wrap_type::k_mirrored_repeat      },
    { "repeat",               wrap_type::k_repeat               },
    { "mirror_clamp_to_edge", wrap_type::k_mirror_clamp_to_edge }
  };

  const auto parse{[this](const string& _type) -> optional<wrap_type> {
    for (const auto& match : k_matches) {
      if (_type == match.match) {
        return match.type;
      }
    }
    error("invalid wrap type '%s'", _type);
    return nullopt;
  }};


  const auto& s_wrap{parse(_wrap[0_z].as_string())};
  const auto& t_wrap{parse(_wrap[1_z].as_string())};

  if (s_wrap && t_wrap) {
    m_wrap.s = *s_wrap;
    m_wrap.t = *t_wrap;
    return true;
  }

  return false;
}

void texture::write_log(log::level _level, string&& _message) const {
  if (m_type.is_empty()) {
    logger(_level, "%s", utility::move(_message));
  } else {
    logger(_level, "%s: %s", m_type, utility::move(_message));
  }
}

} // namespace rx::material