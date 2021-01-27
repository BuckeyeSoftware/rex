#include "rx/render/frontend/material.h"
#include "rx/render/frontend/context.h"
#include "rx/render/frontend/texture.h"

#include "rx/texture/loader.h"

namespace Rx::Render::Frontend {

RX_LOG("render/material", logger);

// This cannot fail as it'll be in-situ.
static String hash_as_string(const Array<Byte[16]>& _hash) {
  static constexpr const auto HEX = "0123456789abcdef";
  String result;
  for (Size i = 0; i < 16; i++) {
    // NOTE(dweiler): These appends will never fail.
    (void)result.append(HEX[(_hash[i] >> 4) & 0x0f]);
    (void)result.append(HEX[_hash[i] & 0x0f]);
  }
  return result;
}

// Check if the bitmap data actually uses the alpha channel.
static bool uses_alpha(const Byte* _data, Size _size) {
  for (Size i = 0; i < _size; i+= 4) {
    if (_data[i + 3] != 255) {
      return true;
    }
  }
  return false;
}

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
  , m_flags{0}
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

bool Material::load(const Rx::Material::Loader& _loader) {
  m_name = _loader.name();
  m_roughness_value = _loader.roughness();
  m_metalness_value = _loader.metalness();
  m_occlusion_value = _loader.occlusion();
  m_albedo_color = _loader.albedo();
  m_emission_color = _loader.emission();
  m_transform = _loader.transform();

  if (_loader.alpha_test()) {
    m_flags |= ALPHA_TEST;
  }

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
    { &m_ambient,    "occlusion", false },
    { &m_emissive,   "emissive",  false }
  };

  return _loader.textures().each_fwd([this, &table](const Rx::Material::Texture& _texture) {
    const auto& type = _texture.type();

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

    const auto& bitmap = _texture.bitmap();

    const auto& hash = hash_as_string(bitmap.hash);

    // Check if cached.
    auto texture = m_frontend->cached_texture2D(hash);
    if (!texture) {
      const auto& filter = _texture.filter();

      // Create a mipmap chain of the texture.
      Rx::Texture::Chain chain;

      // Determine the pixel format we want for the chain.
      Rx::Texture::PixelFormat want_format;
      if (find->srgb) {
        if (Rx::Texture::has_alpha_channel(bitmap.format)) {
          want_format = Rx::Texture::PixelFormat::SRGBA_U8;
        } else {
          want_format = Rx::Texture::PixelFormat::SRGB_U8;
        }
      } else {
        want_format = bitmap.format;
      }

      if (!chain.generate(bitmap.data.data(), bitmap.format,
        want_format, bitmap.dimensions, false, filter.mipmaps))
      {
        return false;
      }

      // Create the texture.
      texture = m_frontend->create_texture2D(RX_RENDER_TAG("material"));

      // Map the Rx::Texture::PixelFormat to a Rx::Render::Frontend::Texture::DataFormat.
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
      case Rx::Texture::PixelFormat::SRGBA_U8:
        texture->record_format(Texture::DataFormat::SRGBA_U8);
        break;
      case Rx::Texture::PixelFormat::SRGB_U8:
        texture->record_format(Texture::DataFormat::SRGB_U8);
        break;
      case Rx::Texture::PixelFormat::R_U8:
        texture->record_format(Texture::DataFormat::R_U8);
        break;
      case Rx::Texture::PixelFormat::RGBA_F32:
        texture->record_format(Texture::DataFormat::RGBA_F32);
        break;
      }

      texture->record_type(Texture::Type::STATIC);
      texture->record_levels(chain.levels().size());
      texture->record_dimensions(chain.dimensions());
      texture->record_filter({filter.bilinear, filter.trilinear, filter.mipmaps});
      texture->record_wrap(convert_material_wrap(_texture.wrap()));

      if (const auto border = _texture.border()) {
        texture->record_border(*border);
      }

      const auto& levels = chain.levels();
      for (Size i = 0; i < levels.size(); i++) {
        const auto& level = levels[i];
        texture->write(chain.data().data() + level.offset, i);
      }

      // Initialize and cache it for reuse.
      m_frontend->initialize_texture(RX_RENDER_TAG("material"), texture);
      m_frontend->cache_texture(texture, hash);
    }

    if (type == "albedo") {
      const auto& level = texture->info_for_level(texture->levels() - 1);
      const auto use_alpha = texture->has_alpha()
        && uses_alpha(texture->data().data() + level.offset, level.size);
      if (use_alpha) {
        m_flags |= HAS_ALPHA;
      } else if (m_flags & ALPHA_TEST) {
        logger->warning("'alpha_test' disabled (\"albedo\" has no alpha channel)");
        m_flags &= ~ALPHA_TEST;
      }
    }

    // Store it.
    *find->texture = texture;

    return true;
  });
}

} // namespace Rx::Render::Frontend
