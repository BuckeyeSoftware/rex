#ifndef RX_IMAGE_CONVERT_H
#define RX_IMAGE_CONVERT_H
#include "rx/core/vector.h"
#include "rx/math/vec2.h"

namespace Rx::Image {

struct Matrix;

// Utilities to convert 8-bit texture data to float image matrix and vise-versa
bool convert(const Matrix& _matrix, Vector<Byte>& data_);
bool convert(const Byte* _data, const Math::Vec2z& _dimensions,
             Size _channels, Matrix& matrix_);

inline bool convert(const Vector<Byte>& _data, const Math::Vec2z& _dimensions,
                    Size _channels, Matrix& matrix_)
{
  return convert(_data.data(), _dimensions, _channels, matrix_);
}

} // namespace Rx::Image

#endif // RX_IMAGE_CONVERT_H
