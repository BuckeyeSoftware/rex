#include <string.h> // memcpy

#include "rx/render/color_grader.h"

#include "rx/render/frontend/context.h"
#include "rx/render/frontend/texture.h"

#include "rx/core/global.h"

#include "rx/core/math/fract.h"

#include "rx/texture/loader.h"

namespace Rx::Render {

using Pixel = Math::Vec4h;

// Generates a Neutral LUT.
struct NeutralLUT {
  static inline constexpr const auto SIZE = ColorGrader::SIZE;

  NeutralLUT() {
    const Math::Vec2z dimensions{SIZE * SIZE, SIZE};
    for (Size y = 0; y < dimensions.h; y++) {
      for (Size x = 0; x < dimensions.w; x++) {
        auto u = Float32(x) / (dimensions.w - 1) * ((dimensions.w - 1.0f) / dimensions.w);
        auto v = Float32(y) / (dimensions.h - 1) * ((dimensions.h - 1.0f) / dimensions.h);

        Math::Vec4f uvw;
        uvw.x = Math::fract(u * SIZE);

        uvw.y = v;
        uvw.z = u - uvw[0] / SIZE;
        uvw.w = 1.0f;

        uvw.x *= SIZE / Float32(SIZE - 1);
        uvw.y *= SIZE / Float32(SIZE - 1);
        uvw.z *= SIZE / Float32(SIZE - 1);

        data[dimensions.w * y + x] = uvw.cast<Math::Half>();
      }
    }
  }
  Pixel data[SIZE * SIZE * SIZE];
  static Global<NeutralLUT> s_instance;
};

Global<NeutralLUT> NeutralLUT::s_instance{"system", "neutral_color_grading_lut"};

// [ColorGrader::LUT]
bool ColorGrader::LUT::read(const String& _file_name) {
  auto& allocator = m_grader->m_frontend->allocator();
  Texture::Loader loader{allocator};

  if (!loader.load(_file_name, Texture::PixelFormat::RGBA_U8, {SIZE * SIZE, SIZE})) {
    return false;
  }

  // Convert to RGBAF16
  Vector<Pixel> data{allocator};
  if (!data.resize(SIZE * SIZE * SIZE)) {
    return false;
  }

  auto src = reinterpret_cast<const Math::Vec4b*>(loader.data().data());
  for (Size i = 0; i < SIZE * SIZE * SIZE; i++) {
    data[i] = (src[i].cast<Float32>() * (1.0f / 255.0f)).cast<Math::Half>();
  }

  // Write the LUT into the atlas.
  write(reinterpret_cast<const Byte*>(data.data()));

  return true;
}

// [ColorGrader]
ColorGrader::ColorGrader(Frontend::Context* _frontend)
  : m_frontend{_frontend}
  , m_texture{nullptr}
  , m_allocated{Utility::move(*Bitset::create(m_frontend->allocator(), MAX_LUTS))}
  , m_dirty{Utility::move(*Bitset::create(m_frontend->allocator(), MAX_LUTS))}
{
}

ColorGrader::~ColorGrader() {
  RX_ASSERT(m_allocated.count_set_bits() == 0, "leaked LUTs");
  m_frontend->destroy_texture(RX_RENDER_TAG("ColorGrader"), m_texture);
}

void ColorGrader::create() {
  m_texture = m_frontend->create_texture3D(RX_RENDER_TAG("ColorGrader"));
  m_texture->record_type(Frontend::Texture::Type::DYNAMIC);
  m_texture->record_levels(1);
  m_texture->record_filter({true, false, false});
  m_texture->record_format(Frontend::Texture::DataFormat::k_rgba_f16);
  m_texture->record_dimensions({SIZE, SIZE, MAX_DEPTH});
  m_texture->record_wrap({
    Render::Frontend::Texture::WrapType::k_clamp_to_edge,
    Render::Frontend::Texture::WrapType::k_clamp_to_edge,
    Render::Frontend::Texture::WrapType::k_clamp_to_edge});

  m_frontend->initialize_texture(RX_RENDER_TAG("ColorGrader"), m_texture);
}

void ColorGrader::update() {
  // Nothing to update.
  if (m_dirty.count_set_bits() == 0) {
    return;
  }

  // Record edits on the frontend and update.
  m_dirty.each_set([&](Bitset::BitType _texture) {
    m_texture->record_edit(0, {0, 0, SIZE * _texture}, {SIZE, SIZE, SIZE});
  });
  m_frontend->update_texture(RX_RENDER_TAG("ColorGrader"), m_texture);

  // No more dirty LUTs.
  m_dirty.clear();
}

Optional<ColorGrader::LUT> ColorGrader::allocate() {
  auto find = m_allocated.find_first_unset();
  if (!find) {
    return nullopt;
  }

  m_allocated.set(*find);

  // Set the neutral data on the LUT.
  write(*find, reinterpret_cast<const Byte*>(NeutralLUT::s_instance->data));

  return LUT{this, Uint32(*find)};
}

// Write |_data| to the LUT given by |_handle|.
void ColorGrader::write(Uint32 _handle, const Byte* _data) {
  // Need to swizzle Z and Y because 3D textures are vertical rather than horizontal.
  auto src = reinterpret_cast<const Pixel*>(_data);
  auto dst = reinterpret_cast<Pixel*>(m_texture->map(0));
  for (Size z = 0; z < SIZE; z++) {
    for (Size y = 0; y < SIZE; y++) {
      memcpy(dst + (z + SIZE * _handle) * SIZE * SIZE + y * SIZE, src + z * SIZE + y * SIZE * SIZE, sizeof(Pixel) * SIZE);
    }
  }

  // This code is what would be used if we didn't need to swizzle incoming.
  //
  // auto src = _data;
  // auto dst = m_texture->map(0) + (SIZE * _handle * SIZE * SIZE) * sizeof(Pixel);
  // auto len = SIZE * SIZE * sizeof(Pixel);
  // for (Size z = 0; z < SIZE; z++) {
  //   memcpy(dst, src, len);
  //   src += len;
  //   dst += len;
  // }

  m_dirty.set(_handle);
}

} // namespace Rx::Render
