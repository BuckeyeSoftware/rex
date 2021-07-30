#ifndef RX_CORE_MATH_CEIL_H
#define RX_CORE_MATH_CEIL_H
#include "rx/core/types.h"

/// file ceil.h
namespace Rx::Math {

/// @{
/// \brief Compute the smallest integer value not less than arg.
/// \param _x Floating point value.
/// \returns The smallest interger value not less than \p _x.
RX_API Float64 ceil(Float64 _x);
RX_API Float32 ceil(Float32 _x);
/// @}

} // namespace Rx::Math

#endif // RX_CORE_MATH_CEIL_H
