#ifndef RX_IMAGE_BRIGHTNESS_CONTRAST_H
#define RX_IMAGE_BRIGHTNESS_CONTRAST_H
#include "rx/core/types.h"
#include "rx/core/algorithm/clamp.h"

#include "rx/image/operation.h"

namespace Rx::Image {

struct BrightnessContrast
  : Operation
{
  struct Options {
    Float64 brightness; // (in [-1, 1])
    Float64 contrast;   // (in [-1, 1])
  };

  void configure(const Options& _options);
  bool process(const Matrix& _src, Matrix& dst_);

private:
  Options m_options;
};

inline void BrightnessContrast::configure(const Options& _options) {
  using Algorithm::clamp;

  m_options.brightness = clamp(_options.brightness, -1.0, 1.0);
  m_options.contrast = clamp(_options.contrast, -1.0, 1.0);
}

} // namespace Rx::Image

#endif // RX_IMAGE_BRIGHTNESS_CONTRAST_H
