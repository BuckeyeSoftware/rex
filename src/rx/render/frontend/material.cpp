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

bool material::load(rx::material::loader&& loader_) {
  m_name = utility::move(loader_.name());
  m_alpha_test = loader_.alpha_test();
  m_has_alpha = loader_.has_alpha();
  m_transform = loader_.transform();

  // Simple table to map type strings to texture2D destinations in this object.
  struct entry {
    texture2D** texture;
    const char* match;
    bool        srgb;
  } table[] {
    { &m_diffuse,    "diffuse",   true  },
    { &m_normal,     "normal",    false },
    { &m_metalness,  "metalness", false },
    { &m_roughness,  "roughness", false }
  };

  return loader_.textures().each_fwd([this, &table](rx::material::texture& texture_) {
    const auto& type{texture_.type()};

    // Search for the texture in the table.
    const entry* find{nullptr};
    for (const auto& item : table) {
      if (item.match == type) {
        find = &item;
      }
    }

    // The texture destination could not be found or we already have a texture
    // constructed in that place.
    if (!find || *find->texture) {
      return false;
    }

    rx::texture::chain&& chain{utility::move(texture_.chain())};
    const auto& filter{texture_.filter()};
    const auto& wrap{texture_.wrap()};

    texture2D* texture{m_frontend->create_texture2D(RX_RENDER_TAG("material"))};
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

    texture->record_type(texture::type::k_static);
    texture->record_levels(chain.levels().size());
    texture->record_dimensions(chain.dimensions());
    texture->record_filter({filter.bilinear, filter.trilinear, filter.mipmaps});
    texture->record_wrap(convert_material_wrap(wrap));
    if (const auto border{texture_.border()}) {
      texture->record_border(*border);
    }

    const auto& levels{chain.levels()};
    for (rx_size i{0}; i < levels.size(); i++) {
      const auto& level{levels[i]};
      texture->write(chain.data().data() + level.offset, i);
    }

    m_frontend->initialize_texture(RX_RENDER_TAG("material"), texture);

    // Store it.
    *find->texture = texture;

    return true;
  });
}

} // namespace rx::render::frontend
