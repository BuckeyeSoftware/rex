#include <string.h> // memcpy

#include "rx/texture/loader.h"
#include "rx/texture/scale.h"

#include "rx/core/filesystem/file.h"
#include "rx/core/log.h"
#include "rx/core/stream.h"

#include "lib/stb_image.h"

#include "rx/console/interface.h"

RX_LOG("texture/loader", logger);

namespace rx::texture {

bool loader::load(stream* _stream, pixel_format _want_format,
  const math::vec2z& _max_dimensions)
{
  auto data = read_binary_stream(m_allocator, _stream);
  if (!data) {
    return false;
  }

  math::vec2<int> dimensions;

  int want_channels{0};
  switch (_want_format) {
  case pixel_format::k_r_u8:
    want_channels = 1;
    break;
  case pixel_format::k_rgb_u8:
    [[fallthrough]];
  case pixel_format::k_bgr_u8:
    want_channels = 3;
    break;
  case pixel_format::k_rgba_u8:
    [[fallthrough]];
  case pixel_format::k_bgra_u8:
    want_channels = 4;
    break;
  }

  int channels;
  rx_byte* decoded_image{stbi_load_from_memory(
    data->data(),
    static_cast<int>(data->size()),
    &dimensions.w,
    &dimensions.h,
    &channels,
    want_channels)};

  if (!decoded_image) {
    logger(log::level::k_error, "%s failed %s", _stream->name(), stbi_failure_reason());
    return false;
  }

  m_channels = want_channels;
  m_bpp = want_channels;
  m_dimensions = dimensions.cast<rx_size>();

  // Scale the texture down if it exceeds the max dimensions.
  if (m_dimensions > _max_dimensions) {
    m_dimensions = _max_dimensions;
    m_data.resize(m_dimensions.area() * m_bpp, utility::uninitialized{});
    scale(decoded_image, dimensions.w, dimensions.h, want_channels,
      dimensions.w * want_channels, m_data.data(), _max_dimensions.w,
      _max_dimensions.h);
  } else {
    // Otherwise just copy the decoded image data directly.
    m_data.resize(m_dimensions.area() * m_bpp, utility::uninitialized{});
    memcpy(m_data.data(), decoded_image, m_data.size());
  }

  stbi_image_free(decoded_image);

  // When there's an alpha channel but it encodes fully opaque, remove it.
  if (want_channels == 4) {
    bool can_remove_alpha{true};
    for (rx_size y{0}; y < m_dimensions.h; y++) {
      const rx_size scan_line_offset{m_dimensions.w * y};
      for (rx_size x{0}; x < m_dimensions.w; x++) {
        // When an alpha pixel has a value other than 255, we can't optimize.
        if (m_data[(scan_line_offset + x) * channels + 3] != 255) {
          can_remove_alpha = false;
          break;
        }
      }
    }

    if (can_remove_alpha) {
      // Remove alpha channel from |m_data|.
      const rx_byte* src = m_data.data();
      rx_byte* dst = m_data.data();
      for (rx_size i = 0; i < m_dimensions.area(); i++) {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        dst += 3;
        src += 4;
      }

      m_channels = 3;
      m_bpp = 3;
      m_data.resize(m_dimensions.area() * m_bpp);

      logger(log::level::k_info, "%s removed alpha channel (not used)",
        _stream->name());
    }
  }

  // When the requested pixel format is a BGR format go ahead and inplace
  // swap the pixels. We cannot use convert for this because we want this
  // to behave inplace.
  if (_want_format == pixel_format::k_bgr_u8
    || _want_format == pixel_format::k_bgra_u8)
  {
    const rx_size samples{m_dimensions.area() * static_cast<rx_size>(channels)};
    for (rx_size i{0}; i < samples; i += channels) {
      const auto r{m_data[0]};
      const auto b{m_data[2]};
      m_data[0] = b;
      m_data[2] = r;
    }
  }

  logger(log::level::k_verbose, "%s loaded %zux%zu @ %zu bpp", _stream->name(),
    m_dimensions.w, m_dimensions.h, m_bpp);

  return true;
}

bool loader::load(stream* _stream, pixel_format _want_format) {
  static auto* max_dimensions_reference =
    console::interface::find_variable_by_name("render.max_texture_dimensions");
  static auto& max_dimensions_variable =
    max_dimensions_reference->cast<math::vec2i>()->get();
  return load(_stream, _want_format, max_dimensions_variable.cast<rx_size>());
}

bool loader::load(const string& _file_name, pixel_format _want_format) {
  if (filesystem::file file{_file_name, "rb"}) {
    return load(&file, _want_format);
  }
  return false;
}

bool loader::load(const string& _file_name, pixel_format _want_format,
  const math::vec2z& _max_dimensions)
{
  if (filesystem::file file{_file_name, "rb"}) {
    return load(&file, _want_format, _max_dimensions);
  }
  return false;
}

} // namespace rx::texture
