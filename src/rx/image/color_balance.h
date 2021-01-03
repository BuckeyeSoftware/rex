#ifndef RX_IMAGE_COLOR_BALANCE_H
#define RX_IMAGE_COLOR_BALANCE_H
#include "rx/core/types.h"
#include "rx/core/algorithm/clamp.h"

#include "rx/image/operation.h"

namespace Rx::Image {

struct Matrix;

struct ColorBalance {
  struct Options {
    Float64 cr[3]; // {SHADOWS, MIDTONES, HIGHLIGHTS}.
    Float64 mg[3]; // {SHADOWS, MIDTONES, HIGHLIGHTS}.
    Float64 yb[3]; // {SHADOWS, MIDTONES, HIGHLIGHTS}.
    bool preserve_luminosity;
  };

  void configure(const Options& _options);
  bool process(const Matrix& _src, Matrix& dst_);

private:
  Options m_options;
};

inline void ColorBalance::configure(const ColorBalance::Options& _options) {
  using Algorithm::clamp;

  for (Size i = 0; i < 3; i++) {
    m_options.cr[i] = clamp(m_options.cr[i], -1.0, 1.0);
    m_options.mg[i] = clamp(m_options.mg[i], -1.0, 1.0);
    m_options.yb[i] = clamp(m_options.yb[i], -1.0, 1.0);
  }

  m_options.preserve_luminosity = _options.preserve_luminosity;
}

} // namespace Rx::Image

#endif // RX_IMAGE_COLOR_BALANCE_H
