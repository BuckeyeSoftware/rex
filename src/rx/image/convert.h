#ifndef RX_IMAGE_CONVERT_H
#define RX_IMAGE_CONVERT_H
#include "rx/core/vector.h"
#include "rx/math/vec2.h"

namespace rx::image {

struct matrix;

// Utilities to convert 8-bit texture data to float image matrix and vise-versa
bool convert(const matrix& _matrix, vector<rx_byte>& data_);
bool convert(const rx_byte* _data, const math::vec2z& _dimensions,
  rx_size _channels, matrix& matrix_);

inline bool convert(const vector<rx_byte>& _data, const math::vec2z& _dimensions,
  rx_size _channels, matrix& matrix_)
{
  return convert(_data.data(), _dimensions, _channels, matrix_);
}

} // namespace rx::image

#endif // RX_IMAGE_CONVERT_H
