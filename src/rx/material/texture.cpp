#include "rx/core/filesystem/file.h"
#include "rx/core/algorithm/clamp.h"
#include "rx/core/math/log2.h"
#include "rx/core/json.h"

#include "rx/math/transform.h"

#include "rx/material/texture.h"

#include "rx/texture/loader.h"
#include "rx/texture/chain.h"

#include "rx/image/normal_map.h"
#include "rx/image/convert.h"
#include "rx/image/matrix.h"

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
    want_format = Rx::Texture::PixelFormat::k_rgba_u8;
  } else if (m_type == "metalness" || m_type == "roughness") {
    want_format = Rx::Texture::PixelFormat::k_r_u8;
  } else {
    want_format = Rx::Texture::PixelFormat::k_rgb_u8;
  }

  Rx::Texture::Loader loader{allocator()};
  if (!loader.load(m_file, want_format, _max_dimensions)) {
    return false;
  }

  // TODO(dweiler): Reintroduce this in a smarter way that the contents of the
  // bake can be serialized.
#if 0
  const auto& generate{_definition["generate"]};
  if (generate && m_type == "normal") {
    if (!generate.is_object()) {
      return error("expected Object for 'generate'");
    }

    const auto& mode{generate["mode"]};
    const auto& multiplier{generate["multiplier"]};
    const auto& kernel{generate["kernel"]};
    const auto& strength{generate["strength"]};
    const auto& flags{generate["flags"]};

    if (!mode)       return error("missing 'mode' for 'generate'");
    if (!multiplier) return error("missing 'multiplier' for 'generate'");
    if (!kernel)     return error("missing 'kernel' for 'generate'");
    if (!strength)   return error("missing 'strength' for 'generate'");
    if (!flags)      return error("missing 'flags' for 'generate'");

    if (!mode.is_string()) {
      return error("expected String for 'mode'");
    }

    if (!multiplier.is_array_of(json::Type::k_number, 3)) {
      return error("expected Array[Number, 3] for 'multiplier'");
    }

    if (!kernel.is_string()) {
      return error("expected String for 'kernel'");
    }

    if (!strength.is_number()) {
      return error("expected Number for 'strength'");
    }

    if (!flags.is_array_of(json::Type::k_string)) {
      return error("expected Array[String] for 'flags'");
    }

    image::intensity_map::mode mode_enum;
    const auto& mode_string{mode.as_string_with_allocator(m_allocator)};
    if (mode_string == "average") {
      mode_enum = image::intensity_map::mode::k_average;
    } else if (mode_string == "max") {
      mode_enum = image::intensity_map::mode::k_max;
    } else {
      return error("invalid 'mode' for 'generate'");
    }

    image::normal_map::kernel kernel_enum;
    const auto& kernel_string{kernel.as_string_with_allocator(m_allocator)};
    if (kernel_string == "sobel") {
      kernel_enum = image::normal_map::kernel::k_sobel;
    } else if (kernel_string == "prewitt") {
      kernel_enum = image::normal_map::kernel::k_prewitt;
    } else {
      return error("invalid 'kernel' for 'generate'");
    }

    int flags_bitset{0};
    const bool result{flags.each([&](const json& _definition) {
      const auto& flag{_definition.as_string_with_allocator(m_allocator)};
      if (flag == "detail") {
        flags_bitset |= image::normal_map::k_detail;
      } else if (flag == "invert") {
        flags_bitset |= image::normal_map::k_invert;
      } else if (flag == "tile") {
        flags_bitset |= image::normal_map::k_tile;
      } else {
        return error("invalid flag '%s' for 'flags'", flag);
      }
      return true;
    })};
    if (!result) {
      return false;
    }

    const math::vec4f multipler_vector{
      multiplier[0_z].as_float(),
      multiplier[1_z].as_float(),
      multiplier[2_z].as_float(),
      0.0f};

    // Convert loader data to image matrix.
    image::matrix matrix{allocator()};
    if (!image::convert(loader.data(), loader.dimensions(), loader.channels(), matrix)) {
      return false;
    }

    // Generate a normal map.
    rx::image::normal_map normal_map{matrix};
    matrix = normal_map.generate(
      mode_enum,
      multipler_vector,
      kernel_enum,
      strength.as_float(),
      flags_bitset);

    // Convert normal map to data.
    LinearBuffer data{allocator()};
    if (!Image::convert(matrix, data)) {
      return false;
    }

    const Rx::Texture::PixelFormat format{loader.format()};
    m_chain.generate(utility::move(data), format, format,
      loader.dimensions(), false, has_mipmaps);
  }
#endif

  m_chain.generate(Utility::move(loader), false, m_filter.mipmaps);

  return true;
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
