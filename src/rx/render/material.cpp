#include <rx/render/material.h>
#include <rx/render/frontend.h>
#include <rx/render/texture.h>

#include <rx/core/filesystem/file.h>
#include <rx/core/debug.h> // RX_MESSAGE

#include <rx/texture/loader.h>
#include <rx/texture/chain.h>

#include <string.h>

RX_LOG("render/material", log_material);

namespace rx::render {

material::material(frontend* _frontend)
  : m_frontend{_frontend}
  , m_diffuse{nullptr}
  , m_normal{nullptr}
  , m_metalness{nullptr}
  , m_roughness{nullptr}
  , m_name{m_frontend->allocator()}
{
}

material::~material() {
  fini();
}

void material::write_log(log::level _level, string&& _message) const {
  if (m_name.is_empty()) {
    log_material(_level, "%s", _message);
  } else {
    log_material(_level, "material '%s': %s", m_name, _message);
  }
}

bool material::load(const string& _file_name) {
  auto data{filesystem::read_binary_file(_file_name)};
  if (!data) {
    return false;
  }

  return parse({data->release()});
}

void material::fini() {
  const auto& tag{RX_RENDER_TAG("finalizer")};

  m_frontend->destroy_texture(tag, m_diffuse);
  m_frontend->destroy_texture(tag, m_normal);
  m_frontend->destroy_texture(tag, m_metalness);
  m_frontend->destroy_texture(tag, m_roughness);
}

bool material::parse(const json& _description) {
  if (!_description) {
    const auto json_error{_description.error()};
    if (json_error) {
      return error("%s", *json_error);
    } else {
      return error("empty description");
    }
  }

  const auto& name{_description["name"]};
  if (!name) {
    return error("missing 'name'");
  }

  if (!name.is_string()) {
    return error("expected String for 'name'");
  }

  m_name = name.as_string();

  const auto& textures{_description["textures"]};
  if (!textures) {
    return error("missing 'textures'");
  }

  if (!textures.is_array_of(json::type::k_object)) {
    return error("expected Array[Object] for 'textures'");
  }

  return textures.each([this](const json& _texture) {
    return parse_texture(_texture);
  });
}

bool material::parse_texture(const json& _texture) {
  const auto& file{_texture["file"]};
  const auto& type{_texture["type"]};
  const auto& filter{_texture["filter"]};
  const auto& wrap{_texture["wrap"]};
  const auto& mipmaps{_texture["mipmaps"]};

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

  if (!type.is_string()) {
    return error("expected String for 'type'");
  }

  if (mipmaps && !mipmaps.is_boolean()) {
    return error("expected Boolean for 'mipmaps'");
  }

  texture2D** store_texture{nullptr};
  const char* tag_string{nullptr};
  const auto& type_string{type.as_string()};
  if (type_string == "diffuse") {
    store_texture = &m_diffuse;
    tag_string = "material diffuse";
  } else if (type_string == "normal") {
    store_texture = &m_normal;
    tag_string = "material normal";
  } else if (type_string == "metalness") {
    store_texture = &m_metalness;
    tag_string = "material metalness";
  } else if (type_string == "roughness") {
    store_texture = &m_roughness;
    tag_string = "material roughness";
  } else {
    return error("invalid texture type '%s'", type_string);
  }

  texture2D* texture{m_frontend->create_texture2D(RX_RENDER_TAG(tag_string))};

  // static texture, cannot be modified
  texture->record_type(texture::type::k_static);

  if (!parse_filter(texture, filter, mipmaps ? mipmaps.as_boolean() : false) || !parse_wrap(texture, wrap)) {
    m_frontend->destroy_texture(RX_RENDER_TAG(tag_string), texture);
    return false;
  }

  rx::texture::loader texture_loader;
  if (texture_loader.load(file.as_string())) {
    rx::texture::chain texture_chain{utility::move(texture_loader), texture->filter().mipmaps};
    const auto& levels{texture_chain.levels()};

    // record the texture format, requires conversion from chain loaded
    switch (texture_chain.format()) {
    case rx::texture::chain::pixel_format::k_rgba_u8:
      texture->record_format(texture::data_format::k_rgba_u8);
      break;
    case rx::texture::chain::pixel_format::k_bgra_u8:
      texture->record_format(texture::data_format::k_bgra_u8);
      break;
    case rx::texture::chain::pixel_format::k_rgb_u8:
      // TODO(dweiler): convert the texture chain to RGBA
      RX_MESSAGE("rgb u8");
      break;
    case rx::texture::chain::pixel_format::k_bgr_u8:
      // TODO(dweiler): convert the texture chain to BGRA
      RX_MESSAGE("bgr u8");
      break;
    case rx::texture::chain::pixel_format::k_r_u8:
      texture->record_format(texture::data_format::k_r_u8);
      break;
    }

    // record the texture dimensions
    texture->record_dimensions(texture_chain.dimensions());

    // write each level from the texture chain into the render resource
    for (rx_size i{0}; i < levels.size(); i++) {
      const auto& level{levels[i]};
      texture->write(texture_chain.data().data() + level.offset, i);
    }
  } else {
    m_frontend->destroy_texture(RX_RENDER_TAG(tag_string), texture);
    return false;
  }

  m_frontend->initialize_texture(RX_RENDER_TAG(tag_string), texture);

  *store_texture = texture;

  return true;
}

bool material::parse_filter(texture2D* texture_, const json& _filter, bool _mipmaps) {
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
      bool trilinear{*match == 't'};
      texture_->record_filter({*match == 'b', trilinear, _mipmaps || trilinear});
      return true;
    }
  }

  return error("unknown filter '%s'", filter_string);
}

bool material::parse_wrap(texture2D* texture_, const json& _wrap) {
  if (!_wrap.is_array_of(json::type::k_string) || _wrap.size() != 2) {
    return error("expected Array[String, 2]");
  }

  static constexpr const struct {
    const char* match;
    texture::wrap_type type;
  } k_matches[]{
    { "clamp_to_edge",        texture::wrap_type::k_clamp_to_edge        },
    { "clamp_to_border",      texture::wrap_type::k_clamp_to_border      },
    { "mirrored_repeat",      texture::wrap_type::k_mirrored_repeat      },
    { "repeat",               texture::wrap_type::k_repeat               },
    { "mirror_clamp_to_edge", texture::wrap_type::k_mirror_clamp_to_edge }
  };

  const auto parse{[this](const string& _type) -> optional<texture::wrap_type> {
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
    texture_->record_wrap({*s_wrap, *t_wrap});
    return true;
  }

  return false;
}

} // namespace rx::render