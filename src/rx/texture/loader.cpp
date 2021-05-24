#include "rx/texture/loader.h"
#include "rx/texture/scale.h"

#include "rx/core/filesystem/unbuffered_file.h"
#include "rx/core/log.h"

#define STBI_NO_BMP
#define STBI_NO_PSD
#define STBI_NO_GIF
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STBI_NO_STDIO

#include "lib/stb_image.h"

RX_LOG("texture/loader", logger);

namespace Rx::Texture {

bool Loader::load(Stream::UntrackedStream& _stream, PixelFormat _want_format,
  const Math::Vec2z& _max_dimensions)
{
  auto data = _stream.read_binary(allocator());
  if (!data) {
    return false;
  }

  Math::Vec2<int> dimensions;

  int want_channels = 0;
  switch (_want_format) {
  case PixelFormat::R_U8:
    want_channels = 1;
    break;
  case PixelFormat::RGB_U8:
    [[fallthrough]];
  case PixelFormat::BGR_U8:
    [[fallthrough]];
  case PixelFormat::SRGB_U8:
    want_channels = 3;
    break;
  case PixelFormat::RGBA_U8:
    [[fallthrough]];
  case PixelFormat::BGRA_U8:
    [[fallthrough]];
  case PixelFormat::SRGBA_U8:
    want_channels = 4;
    break;
  case PixelFormat::RGBA_F32:
    want_channels = 4;
    break;
  }

  int decoded_channels = 0;
  Byte* decoded_image = nullptr;
  if (_want_format == PixelFormat::RGBA_F32) {
    decoded_image = reinterpret_cast<Byte*>(
      stbi_loadf_from_memory(
        data->data(),
        static_cast<int>(data->size()),
        &dimensions.w,
        &dimensions.h,
        &decoded_channels,
        want_channels));
  } else {
    decoded_image = stbi_load_from_memory(
      data->data(),
      static_cast<int>(data->size()),
      &dimensions.w,
      &dimensions.h,
      &decoded_channels,
      want_channels);
  }

  if (!decoded_image) {
    logger->error("%s failed %s", _stream.name(), stbi_failure_reason());
    stbi_image_free(decoded_image);
    return false;
  }

  m_format = _want_format;
  m_channels = want_channels;
  m_dimensions = dimensions.cast<Size>();

  // Scale the texture down if it exceeds the max dimensions.
  // TODO(dweiler): This only works for non-float formats.
  bool resize = false;
  if (m_dimensions > _max_dimensions && !is_float_format(_want_format)) {
    m_dimensions = _max_dimensions;
    resize = true;
  }

  if (!m_data.resize(m_dimensions.area() * bits_per_pixel() / 8)) {
    stbi_image_free(decoded_image);
    return false;
  }

  if (resize) {
    scale(decoded_image, dimensions.w, dimensions.h, want_channels,
      dimensions.w * want_channels, m_data.data(), _max_dimensions.w,
      _max_dimensions.h);
  } else {
    Memory::copy(m_data.data(), decoded_image, m_data.size());
  }

  stbi_image_free(decoded_image);

  // When the requested pixel format is a BGR format go ahead and inplace
  // swap the pixels. We cannot use convert for this because we want this
  // to behave inplace.
  if (_want_format == PixelFormat::BGR_U8
    || _want_format == PixelFormat::BGRA_U8)
  {
    const Size samples =
      m_dimensions.area() * static_cast<Size>(m_channels);

    for (Size i = 0; i < samples; i += m_channels) {
      const auto r = m_data[0];
      const auto b = m_data[2];
      m_data[0] = b;
      m_data[2] = r;
    }
  }

  logger->verbose("%s loaded %zux%zu @ %zu bpp", _stream.name(),
    m_dimensions.w, m_dimensions.h, bits_per_pixel());

  return true;
}

bool Loader::load(const String& _file_name, PixelFormat _want_format,
  const Math::Vec2z& _max_dimensions)
{
  if (auto file = Filesystem::UnbufferedFile::open(allocator(), _file_name, "r")) {
    return load(*file, _want_format, _max_dimensions);
  }
  return false;
}

Size Loader::bits_per_pixel() const {
  switch (m_format) {
  case PixelFormat::R_U8:
    return 8;
  case PixelFormat::BGR_U8:
    [[fallthrough]];
  case PixelFormat::RGB_U8:
    [[fallthrough]];
  case PixelFormat::SRGB_U8:
    return 8 * 3;
  case PixelFormat::BGRA_U8:
    [[fallthrough]];
  case PixelFormat::RGBA_U8:
    [[fallthrough]];
  case PixelFormat::SRGBA_U8:
    return 8 * 4;
  case PixelFormat::RGBA_F32:
    return 32 * 4;
  }
  RX_HINT_UNREACHABLE();
}

} // namespace Rx::Texture
