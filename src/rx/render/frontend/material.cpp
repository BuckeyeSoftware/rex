#include "rx/render/frontend/material.h"
#include "rx/render/frontend/interface.h"
#include "rx/render/frontend/texture.h"

#include "rx/texture/loader.h"

namespace rx::render::frontend {

static inline texture2D::wrap_options
convert_material_wrap(const rx::material::texture::wrap_options& _wrap) {
  auto convert{[](auto _value) {
    switch (_value) {
    case rx::material::texture::wrap_type::k_clamp_to_edge:
      return texture::wrap_type::k_clamp_to_edge;
    case rx::material::texture::wrap_type::k_clamp_to_border:
      return texture::wrap_type::k_clamp_to_border;
    case rx::material::texture::wrap_type::k_mirrored_repeat:
      return texture::wrap_type::k_mirrored_repeat;
    case rx::material::texture::wrap_type::k_mirror_clamp_to_edge:
      return texture::wrap_type::k_mirror_clamp_to_edge;
    case rx::material::texture::wrap_type::k_repeat:
      return texture::wrap_type::k_repeat;
    }
    RX_HINT_UNREACHABLE();
  }};

  return {convert(_wrap.s), convert(_wrap.t)};
}

material::material(interface* _frontend)
  : m_frontend{_frontend}
  , m_diffuse{nullptr}
  , m_normal{nullptr}
  , m_metalness{nullptr}
  , m_roughness{nullptr}
  , m_alpha_test{false}
  , m_has_alpha{false}
  , m_name{m_frontend->allocator()}
{
}

material::~material() {
  const auto& tag{RX_RENDER_TAG("finalizer")};

  m_frontend->destroy_texture(tag, m_diffuse);
  m_frontend->destroy_texture(tag, m_normal);
  m_frontend->destroy_texture(tag, m_metalness);
  m_frontend->destroy_texture(tag, m_roughness);
}

bool material::load(rx::material::loader&& _loader) {
  m_name = utility::move(_loader.name());
  m_alpha_test = _loader.alpha_test();
  m_has_alpha = _loader.has_alpha();
  m_transform = _loader.transform();

  // Simple table to map type strings to texture2D destinations in this object.
  struct {
    texture2D** texture;
    const char* match;
  } table[] {
    { &m_diffuse,    "diffuse"   },
    { &m_normal,     "normal"    },
    { &m_metalness,  "metalness" },
    { &m_roughness,  "roughness" }
  };

  return _loader.textures().each_fwd([this, &table](rx::material::texture& _texture) {
    const auto& type{_texture.type()};

    // Search for the texture in the table.
    texture2D** destination{nullptr};
    for (const auto& item : table) {
      if (item.match == type) {
        destination = item.texture;
        break;
      }
    }

    // The texture destination could not be found or we already have a texture
    // constructed in that place.
    if (!destination || *destination) {
      return false;
    }

    rx::texture::chain&& chain{utility::move(_texture.chain())};
    const auto& filter{_texture.filter()};
    const auto& wrap{_texture.wrap()};

    texture2D* texture{m_frontend->create_texture2D(RX_RENDER_TAG("material"))};
    texture->record_type(texture::type::k_static);
    texture->record_filter({filter.bilinear, filter.trilinear, filter.mipmaps});
    texture->record_wrap(convert_material_wrap(wrap));

    switch (chain.format()) {
    case rx::texture::chain::pixel_format::k_rgba_u8:
      texture->record_format(texture::data_format::k_rgba_u8);
      break;
    case rx::texture::chain::pixel_format::k_bgra_u8:
      texture->record_format(texture::data_format::k_bgra_u8);
      break;
    case rx::texture::chain::pixel_format::k_rgb_u8:
      texture->record_format(texture::data_format::k_rgb_u8);
      break;
    case rx::texture::chain::pixel_format::k_bgr_u8:
      texture->record_format(texture::data_format::k_bgr_u8);
      break;
    case rx::texture::chain::pixel_format::k_r_u8:
      texture->record_format(texture::data_format::k_r_u8);
      break;
    }

    texture->record_dimensions(chain.dimensions());

    const auto& levels{chain.levels()};
    for (rx_size i{0}; i < levels.size(); i++) {
      const auto& level{levels[i]};
      texture->write(chain.data().data() + level.offset, i);
    }

    m_frontend->initialize_texture(RX_RENDER_TAG("material"), texture);

    // Store it.
    *destination = texture;

    return true;
  });
}

} // namespace rx::render::frontend