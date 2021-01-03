#ifndef RX_IMAGE_OPERATION_H
#define RX_IMAGE_OPERATION_H
#include "rx/core/markers.h"

namespace Rx::Image {

struct Matrix;

struct Operation {
  RX_MARK_INTERFACE(Operation);

  virtual bool process(const Matrix& _src, Matrix& _dst) = 0;
};

} // namespace Rx::Image

#endif // RX_IMAGE_OPERATION_H
