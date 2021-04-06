#ifndef RX_CORE_MATH_ABS_H
#define RX_CORE_MATH_ABS_H
#include "rx/core/types.h"

/// \file abs.h
namespace Rx::Math {

/// @{
/// \brief Compute absolute value.
/// \param _x Value whose absolute value is returned.
/// \returns The absolute value of \p _x.
RX_API Float64 abs(Float64 _x);
RX_API Float32 abs(Float32 _x);
/// @}

} // namespace Rx::Math

#endif // RX_CORE_MATH_ABS_H
