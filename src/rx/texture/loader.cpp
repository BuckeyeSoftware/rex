#include <string.h>

#include <rx/texture/loader.h>
#include <rx/core/filesystem/file.h>

#include "lib/stb_image.h"

namespace rx::texture {

bool loader::load(const string& _file_name) {
  auto read_data{filesystem::read_binary_file(m_allocator, _file_name)};
  if (!read_data) {
    return false;
  }

  const auto& data{*read_data};

  math::vec2<int> dimensions;
  int channels;
  rx_byte* decoded_image{stbi_load_from_memory(data.data(),
    static_cast<int>(data.size()), &dimensions.w, &dimensions.h, &channels,
    0)};
  if (!decoded_image) {
    return false;
  }

  m_dimensions = dimensions.cast<rx_size>();
  m_channels = channels;
  m_bpp = m_channels;

  m_data.resize(m_dimensions.area() * m_bpp);
  memcpy(m_data.data(), decoded_image, m_data.size());
  
  stbi_image_free(decoded_image);

  return true;
}

} // namespace rx::texture