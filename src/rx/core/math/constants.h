#ifndef RX_CORE_MATH_CONSTANTS_H
#define RX_CORE_MATH_CONSTANTS_H

namespace Rx::Math {

// Because of rounding we do these a bit better.
template<typename T>
inline constexpr const T PI = T(3.1415926535897932384626433832795028841971693993751);

template<typename T>
inline constexpr const T PI_2 = T(1.5707963267948966192313216916397514420985846996876);

template<typename T>
inline constexpr const T PI_4 = T(0.78539816339744830961566084581987572104929234984378);

template<typename T>
inline constexpr const T EPSILON = T(0.0001);

} // namespace Rx::Math

#endif // RX_CORE_MATH_CONSTANTS_H
