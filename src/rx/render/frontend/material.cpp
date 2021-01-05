#include "rx/render/frontend/material.h"
#include "rx/render/frontend/context.h"
#include "rx/render/frontend/texture.h"

#include "rx/texture/loader.h"

namespace Rx::Render::Frontend {

static inline Texture2D::WrapOptions
convert_material_wrap(const Rx::Material::Texture::Wrap& _wrap) {
  auto convert = [](auto _value) {
    switch (_value) {
    case Rx::Material::Texture::WrapType::CLAMP_TO_EDGE:
      return Texture::WrapType::CLAMP_TO_EDGE;
    case Rx::Material::Texture::WrapType::CLAMP_TO_BORDER:
      return Texture::WrapType::CLAMP_TO_BORDER;
    case Rx::Material::Texture::WrapType::MIRRORED_REPEAT:
      return Texture::WrapType::MIRRORED_REPEAT;
    case Rx::Material::Texture::WrapType::MIRROR_CLAMP_TO_EDGE:
      return Texture::WrapType::MIRROR_CLAMP_TO_EDGE;
    case Rx::Material::Texture::WrapType::REPEAT:
      return Texture::WrapType::REPEAT;
    }
    RX_HINT_UNREACHABLE();
  };

  return {convert(_wrap.s), convert(_wrap.t)};
}

Material::Material(Context* _frontend)
  : m_frontend{_frontend}
  , m_albedo{nullptr}
  , m_normal{nullptr}
  , m_roughness{nullptr}
  , m_metalness{nullptr}
  , m_ambient{nullptr}
  , m_emissive{nullptr}
  , m_alpha_test{false}
  , m_has_alpha{false}
  , m_roughness_value{1.0f}
  , m_metalness_value{0.0f}
  , m_occlusion_value{1.0f}
  , m_albedo_color{1.0f, 1.0f, 1.0f}
  , m_emission_color{0.0f, 0.0f, 0.0f}
  , m_name{m_frontend->allocator()}
{
}

Material::~Material() {
  const auto& tag = RX_RENDER_TAG("finalizer");

  m_frontend->destroy_texture(tag, m_albedo);
  m_frontend->destroy_texture(tag, m_normal);
  m_frontend->destroy_texture(tag, m_roughness);
  m_frontend->destroy_texture(tag, m_metalness);
  m_frontend->destroy_texture(tag, m_ambient);
  m_frontend->destroy_texture(tag, m_emissive);
}

bool Material::load(Rx::Material::Loader&& loader_) {
  m_name = Utility::move(loader_.name());
  m_alpha_test = loader_.alpha_test();
  m_has_alpha = loader_.has_alpha();
  m_roughness_value = loader_.roughness();
  m_metalness_value = loader_.metalness();
  m_occlusion_value = loader_.occlusion();
  m_albedo_color = loader_.albedo();
  m_emission_color = loader_.emission();
  m_transform = loader_.transform();

  // Simple table to map Type strings to texture2D destinations in this object.
  struct Entry {
    Texture2D** texture;
    const char* match;
    bool        srgb;
  } table[] {
    { &m_albedo,     "albedo",    true  },
    { &m_normal,     "normal",    false },
    { &m_metalness,  "metalness", false },
    { &m_roughness,  "roughness", false },
    { &m_ambient,    "ambient",   false },
    { &m_emissive,   "emissive",  false }
  };

  return loader_.textures().each_fwd([this, &table](Rx::Material::Texture& texture_) {
    const auto& type = texture_.type();

    // Search for the texture in the table.
    const Entry* find = nullptr;
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

    Rx::Texture::Chain chain = Utility::move(texture_.chain());
    const auto& filter = texture_.filter();
    const auto& wrap = texture_.wrap();

    Texture2D* texture =
      m_frontend->create_texture2D(RX_RENDER_TAG("material"));

    switch (chain.format()) {
    case Rx::Texture::PixelFormat::RGBA_U8:
      texture->record_format(Texture::DataFormat::RGBA_U8);
      break;
    case Rx::Texture::PixelFormat::BGRA_U8:
      texture->record_format(Texture::DataFormat::BGRA_U8);
      break;
    case Rx::Texture::PixelFormat::RGB_U8:
      texture->record_format(Texture::DataFormat::RGB_U8);
      break;
    case Rx::Texture::PixelFormat::BGR_U8:
      texture->record_format(Texture::DataFormat::BGR_U8);
      break;
    case Rx::Texture::PixelFormat::R_U8:
      texture->record_format(Texture::DataFormat::R_U8);
      break;
    }

    texture->record_type(Texture::Type::STATIC);
    texture->record_levels(chain.levels().size());
    texture->record_dimensions(chain.dimensions());
    texture->record_filter({filter.bilinear, filter.trilinear, filter.mipmaps});
    texture->record_wrap(convert_material_wrap(wrap));
    if (const auto border = texture_.border()) {
      texture->record_border(*border);
    }

    const auto& levels = chain.levels();
    for (Size i{0}; i < levels.size(); i++) {
      const auto& level = levels[i];
      texture->write(chain.data().data() + level.offset, i);
    }

    m_frontend->initialize_texture(RX_RENDER_TAG("material"), texture);

    // Store it.
    *find->texture = texture;

    return true;
  });
}

} // namespace Rx::Render::Frontend
