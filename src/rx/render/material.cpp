#include <rx/render/material.h>
#include <rx/render/frontend.h>
#include <rx/render/texture.h>

#include <rx/core/filesystem/file.h>

#include <rx/texture/loader.h>
#include <rx/texture/chain.h>

#include <string.h>

RX_LOG("render/material", log_material);

namespace rx::render {

material::material(frontend* _frontend)
  : m_frontend{_frontend}
  , m_diffuse{nullptr}
  , m_normal{nullptr}
  , m_metal{nullptr}
  , m_roughness{nullptr}
  , m_name{m_frontend->allocator()}
{
}

void material::write_log(log::level _level, string&& _message) const {
  if (m_name.is_empty()) {
    log_material(_level, "%s", _message);
  } else {
    log_material(_level, "material '%s': %s", m_name, _message);
  }
}

bool material::load(const string& _file_name) {
  const auto data{filesystem::read_binary_file(_file_name)};
  if (!data) {
    return false;
  }

  string contents{m_frontend->allocator()};
  contents.resize(data->size());
  memcpy(contents.data(), data->data(), data->size());

  return parse({contents});
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
  } else if (type_string == "metal") {
    store_texture = &m_metal;
    tag_string = "material metal";
  } else if (type_string == "roughness") {
    store_texture = &m_roughness;
    tag_string = "material roughness";
  } else {
    return error("invalid texture type '%s'", type_string);
  }

  texture2D* texture{m_frontend->create_texture2D(RX_RENDER_TAG(tag_string))};
  if (!parse_filter(texture, filter, mipmaps ? mipmaps.as_boolean() : false) || !parse_wrap(texture, wrap)) {
    m_frontend->destroy_texture(RX_RENDER_TAG(tag_string), texture);
    return false;
  }

  rx::texture::loader texture_loader;
  if (texture_loader.load(file.as_string())) {
    rx::texture::chain texture_chain{utility::move(texture_loader), texture->filter().mipmaps};
    const auto& levels{texture_chain.levels()};

    // record the texture dimensions and begin writing the chain into the render resource
    texture->record_dimensions(texture_chain.dimensions());
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

  bool bilinear{false};
  bool trilinear{false};
  bool mipmaps{_mipmaps};
  const auto& filter_string{_filter.as_string()};
  if (filter_string == "bilinear") {
    bilinear = true;
  } else if (filter_string == "trilinear") {
    trilinear = true;
    mipmaps = true;
  } else if (filter_string != "nearest") {
    return error("unknown filter '%s'", filter_string);
  }

  texture_->record_filter({bilinear, trilinear, mipmaps});

  return true;
}

bool material::parse_wrap(texture2D* texture_, const json& _wrap) {
  if (!_wrap.is_array_of(json::type::k_string) || _wrap.size() != 2) {
    return error("expected Array[String, 2]");
  }

  const auto parse{[this](const string& _type) -> optional<texture::wrap_type> {
    if (_type == "clamp_to_edge") {
      return texture::wrap_type::k_clamp_to_edge;
    } else if (_type == "clamp_to_border") {
      return texture::wrap_type::k_clamp_to_border;
    } else if (_type == "mirrored_repeat") {
      return texture::wrap_type::k_mirrored_repeat;
    } else if (_type == "repeat") {
      return texture::wrap_type::k_repeat;
    } else if (_type == "mirror_clamp_to_edge") {
      return texture::wrap_type::k_mirror_clamp_to_edge;
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