#ifndef RX_CORE_MATH_CONSTANTS_H
#define RX_CORE_MATH_CONSTANTS_H

namespace Rx::Math {

template<typename T>
inline constexpr const T PI = T(3.14159265358979323846264338327950288);

template<typename T>
inline constexpr const T PI_2 = PI<T> / T{2.0};

template<typename T>
inline constexpr const T PI_4 = PI<T> / T{4.0};

} // namespace Rx::Math

#endif // RX_CORE_MATH_CONSTANTS_H
