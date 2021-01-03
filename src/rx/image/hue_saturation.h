#ifndef RX_IMAGE_HUE_SATURATION_H
#define RX_IMAGE_HUE_SATURATION_H
#include "rx/core/types.h"
#include "rx/core/algorithm/clamp.h"
#include "rx/core/algorithm/saturate.h"

#include "rx/image/operation.h"

namespace Rx::Image {

struct HueSaturation
  : Operation
{
  // The ranges to modify.
  enum {
    ALL,
    RED,
    YELLOW,
    GREEN,
    CYAN,
    BLUE,
    MAGENTA
  };

  struct Options {
    Float64 hue[7];         // {ALL, RED, YELLOW, GREEN, CYAN, BLUE, MAGENTA} (in [-1, 1])
    Float64 saturation[7];  // {ALL, RED, YELLOW, GREEN, CYAN, BLUE, MAGENTA} (in [-1, 1])
    Float64 lightness[7];   // {ALL, RED, YELLOW, GREEN, CYAN, BLUE, MAGENTA} (in [-1, 1])
    Float64 overlap;        // (in [0, 1])
  };

  void configure(const Options& _options);
  bool process(const Matrix& _src, Matrix& dst_);

private:
  Options m_options;
};

inline void HueSaturation::configure(const Options& _options) {
  using Algorithm::clamp;
  using Algorithm::saturate;

  for (Size i = 0; i < 7; i++) {
    m_options.hue[i] = clamp(_options.hue[i], -1.0, 1.0);
    m_options.saturation[i] = clamp(_options.saturation[i], -1.0, 1.0);
    m_options.lightness[i] = clamp(_options.lightness[i], -1.0, 1.0);
  }

  m_options.overlap = saturate(_options.overlap);
}

} // namespace Rx::Image

#endif // RX_IMAGE_HUE_SATURATION
