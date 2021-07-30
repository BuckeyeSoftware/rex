#ifndef RX_CORE_MATH_COS_H
#define RX_CORE_MATH_COS_H
#include "rx/core/types.h"

/// \file cos.h
namespace Rx::Math {

/// \brief Computes the cosine (measured in radians).
/// \param _x Value representing angle in radians.
/// \return The cosine of arg in the range [-1.0, 1.0].
RX_API Float32 cos(Float32 _x);

/// \brief Computes the principal value of the arc cosine.
/// \param _x Value.
/// \return If no error occurs, the arc cosine of \p x in the range [0, pi], is
/// returned. If a domain error occurs, NaN is returned.
/// If a range error occurs due to underflow, the correct result (after rounding)
/// is returned.
RX_API Float32 acos(Float32 _x);

} // namespace Rx::Math

#endif // RX_CORE_MATH_COS_H
