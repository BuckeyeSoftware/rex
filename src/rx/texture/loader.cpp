#include "rx/texture/loader.h"
#include "rx/texture/scale.h"
#include "rx/texture/convert.h"

#include "rx/core/filesystem/unbuffered_file.h"
#include "rx/core/log.h"

#if defined(RX_PLATFORM_EMSCRIPTEN)
// Needed since emscripten_get_preloaded_image_data returned pointer must be freed.
#include <stdlib.h>
#include <emscripten.h>
#endif // defined(RX_PLATFORM_EMSCRIPTEN)

#define STBI_NO_BMP
#define STBI_NO_PSD
#define STBI_NO_GIF
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STBI_NO_STDIO

#include "lib/stb_image.h"

RX_LOG("texture/loader", logger);

namespace Rx::Texture {

bool Loader::load(Stream::Context& _stream, PixelFormat _want_format,
  const Math::Vec2z& _max_dimensions)
{
  LinearBuffer content{allocator()};

  Size want_channels = 0;
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
    [[fallthrough]];
  case PixelFormat::RGBA_F32:
    want_channels = 4;
    break;
  }

#if defined(RX_PLATFORM_EMSCRIPTEN)
  // Use the browser's builtin image decoders.
  Math::Vec2i dimensions;
  auto decoded_image = emscripten_get_preloaded_image_data(_stream.name().data(),
    &dimensions.w, &dimensions.h);
  if (!decoded_image) {
    return false;
  }

  m_format = PixelFormat::RGBA_U8;
  m_channels = 4;
  m_dimensions = dimensions.cast<Size>();
  const auto n_bytes = (m_dimensions.area() * bits_per_pixel()) / 8;
  if (!content.append(reinterpret_cast<const Byte*>(decoded_image), n_bytes)) {
    logger->error("%s out of memory", _stream.name());
    free(decoded_image);
    return false;
  }
  free(decoded_image);
#else
  // Use STBI.
  auto data = _stream.read_binary(allocator());
  if (!data) {
    return false;
  }

  Math::Vec2i dimensions;

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
        Sint32(want_channels)));
  } else {
    decoded_image = stbi_load_from_memory(
      data->data(),
      static_cast<int>(data->size()),
      &dimensions.w,
      &dimensions.h,
      &decoded_channels,
      Sint32(want_channels));
  }

  if (!decoded_image) {
    logger->error("%s failed %s", _stream.name(), stbi_failure_reason());
    stbi_image_free(decoded_image);
    return false;
  }

  // STBI cannot output BGR or BGRA formats, so swap that here.
  m_format =
    _want_format == PixelFormat::BGRA_U8
      ? PixelFormat::RGBA_F32
      : _want_format == PixelFormat::BGR_U8
        ? PixelFormat::RGB_U8
        : _want_format;
  m_channels = want_channels;
  m_dimensions = dimensions.cast<Size>();
  const auto n_bytes = (m_dimensions.area() * bits_per_pixel()) / 8;
  if (!content.append(decoded_image, n_bytes)) {
    logger->error("%s out of memory", _stream.name());
    stbi_image_free(decoded_image);
    return false;
  }
  stbi_image_free(decoded_image);
#endif

  // Scale the texture down if it exceeds the max dimensions, but preserve
  // aspect ratio when doing it.
  //
  // TODO(dweiler): Support resizing float formats.
  if (m_dimensions > _max_dimensions && !is_float_format(_want_format)) {
    const auto factor =
      (_max_dimensions.cast<Float32>() / m_dimensions.cast<Float32>()).min_element();
    const auto max_dimensions =
      (m_dimensions.cast<Float32>() * factor).cast<Size>();
    const auto n_bytes = (m_dimensions.area() * bits_per_pixel()) / 8;
    if (!m_data.resize(n_bytes)) {
      logger->error("%s out of memory", _stream.name());
      return false;
    }
    scale(content.data(), m_dimensions.w, m_dimensions.h, m_channels,
      m_dimensions.w * m_channels, m_data.data(), max_dimensions.w,
      max_dimensions.h);
    m_dimensions = max_dimensions;
  } else {
    m_data = Utility::move(content);
  }

  // Reformat contents when the loaded result does not match the request.
  if (m_channels != want_channels || m_format != _want_format) {
    const auto n_bytes = (m_dimensions.area() * bits_per_pixel()) / 8;
    auto reformat = convert(allocator(), m_data.data(), n_bytes, m_format, _want_format);
    if (!reformat) {
      return false;
    }
    m_data = Utility::move(*reformat);
    m_channels = want_channels;
    m_format = _want_format;
  }

  logger->verbose("%s loaded %zux%zu @ %zu bpp", _stream.name(),
    m_dimensions.w, m_dimensions.h, bits_per_pixel());

  return true;
}

bool Loader::load(const StringView& _file_name, PixelFormat _want_format,
  const Math::Vec2z& _max_dimensions)
{
  if (auto file = Filesystem::UnbufferedFile::open(allocator(), _file_name, "r")) {
    return load(*file, _want_format, _max_dimensions);
  }
  return false;
}

Size Loader::bits_per_pixel() const {
  return Texture::bits_per_pixel(m_format);
}

} // namespace Rx::Texture
