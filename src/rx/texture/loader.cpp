#include <string.h> // memcpy

#include "rx/texture/loader.h"
#include "rx/texture/scale.h"

#include "rx/core/filesystem/file.h"
#include "rx/core/log.h"
#include "rx/core/stream.h"

#include "lib/stb_image.h"

RX_LOG("texture/loader", logger);

namespace Rx::Texture {

bool Loader::load(Stream* _stream, PixelFormat _want_format,
  const Math::Vec2z& _max_dimensions)
{
  auto data = read_binary_stream(allocator(), _stream);
  if (!data) {
    return false;
  }

  Math::Vec2<int> dimensions;

  int want_channels{0};
  switch (_want_format) {
  case PixelFormat::k_r_u8:
    want_channels = 1;
    break;
  case PixelFormat::k_rgb_u8:
    [[fallthrough]];
  case PixelFormat::k_bgr_u8:
    want_channels = 3;
    break;
  case PixelFormat::k_rgba_u8:
    [[fallthrough]];
  case PixelFormat::k_bgra_u8:
    want_channels = 4;
    break;
  }

  int channels;
  Byte* decoded_image{stbi_load_from_memory(
    data->data(),
    static_cast<int>(data->size()),
    &dimensions.w,
    &dimensions.h,
    &channels,
    want_channels)};

  if (!decoded_image) {
    logger->error("%s failed %s", _stream->name(), stbi_failure_reason());
    return false;
  }

  m_channels = want_channels;
  m_bpp = want_channels;
  m_dimensions = dimensions.cast<Size>();

  // Scale the texture down if it exceeds the max dimensions.
  if (m_dimensions > _max_dimensions) {
    m_dimensions = _max_dimensions;
    m_data.resize(m_dimensions.area() * m_bpp, Utility::UninitializedTag{});
    scale(decoded_image, dimensions.w, dimensions.h, want_channels,
      dimensions.w * want_channels, m_data.data(), _max_dimensions.w,
      _max_dimensions.h);
  } else {
    // Otherwise just copy the decoded image data directly.
    m_data.resize(m_dimensions.area() * m_bpp, Utility::UninitializedTag{});
    memcpy(m_data.data(), decoded_image, m_data.size());
  }

  stbi_image_free(decoded_image);

  // When there's an alpha channel but it encodes fully opaque, remove it.
  if (want_channels == 4) {
    bool can_remove_alpha{true};
    for (Size y = 0; y < m_dimensions.h; y++) {
      const Size scan_line_offset{m_dimensions.w * y};
      for (Size x = 0; x < m_dimensions.w; x++) {
        // When an alpha pixel has a value other than 255, we can't optimize.
        if (m_data[(scan_line_offset + x) * channels + 3] != 255) {
          can_remove_alpha = false;
          break;
        }
      }
    }

    if (can_remove_alpha) {
      // Remove alpha channel from |m_data|.
      const Byte* src = m_data.data();
      Byte* dst = m_data.data();
      for (Size i = 0; i < m_dimensions.area(); i++) {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        dst += 3;
        src += 4;
      }

      m_channels = 3;
      m_bpp = 3;
      m_data.resize(m_dimensions.area() * m_bpp);

      logger->info("%s removed alpha channel (not used)", _stream->name());
    }
  }

  // When the requested pixel format is a BGR format go ahead and inplace
  // swap the pixels. We cannot use convert for this because we want this
  // to behave inplace.
  if (_want_format == PixelFormat::k_bgr_u8
    || _want_format == PixelFormat::k_bgra_u8)
  {
    const Size samples =
      m_dimensions.area() * static_cast<Size>(channels);
    for (Size i = 0; i < samples; i += channels) {
      const auto r = m_data[0];
      const auto b = m_data[2];
      m_data[0] = b;
      m_data[2] = r;
    }
  }

  logger->verbose("%s loaded %zux%zu @ %zu bpp", _stream->name(),
    m_dimensions.w, m_dimensions.h, m_bpp);

  return true;
}

bool Loader::load(const String& _file_name, PixelFormat _want_format,
  const Math::Vec2z& _max_dimensions)
{
  if (Filesystem::File file{_file_name, "rb"}) {
    return load(&file, _want_format, _max_dimensions);
  }
  return false;
}

} // namespace rx::texture
