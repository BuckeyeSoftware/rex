#ifndef RX_IMAGE_COLORIZE_H
#define RX_IMAGE_COLORIZE_H
#include "rx/core/types.h"

#include "rx/core/algorithm/saturate.h"
#include "rx/core/algorithm/clamp.h"

#include "rx/image/operation.h"

namespace Rx::Image {

struct Colorize
  : Operation
{
  struct Options {
    Float64 hue        = 0.5f; // (in [0, 1])
    Float64 saturation = 0.5f; // (in [0, 1])
    Float64 lightness  = 0.0f; // (in [-1, 1])
  };

  void configure(const Options& _options);
  bool process(const Matrix& _src, Matrix& dst_);

private:
  Options m_options;
};

inline void Colorize::configure(const Options& _options) {
  using Algorithm::clamp;
  using Algorithm::saturate;

  m_options.hue = saturate(_options.hue);
  m_options.saturation = saturate(_options.saturation);
  m_options.lightness = clamp(_options.lightness, -1.0, 1.0);
}

} // namespace Rx::Image

#endif // RX_IMAGE_COLORIZE_H
