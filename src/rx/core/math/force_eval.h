#ifndef RX_CORE_MATH_FORCE_EVAL_H
#define RX_CORE_MATH_FORCE_EVAL_H
#include "rx/core/types.h"

namespace Rx {

/// @{
/// Force the evaluation of a floating point expression even in the presence of
/// compiler optimization. Depending on the compiler settings certain
/// expressions intended to generate special floating point values like NaNs may
/// be incorrectly optimized breaking code that depends on that behavior.
/// Executingg such expressions as an argument to these functions prevent such
/// optimizations from occuring.
inline void force_eval_f32(Float32 _x) {
  [[maybe_unused]] volatile Float32 y;
  y = _x;
}

inline void force_eval_f64(Float64 _x) {
  [[maybe_unused]] volatile Float64 y;
  y = _x;
}
/// @}

} // namespace Rx

#endif // RX_CORE_MATH_FORCE_EVAL_H
