#ifndef RX_MATH_CONSTANTS_H
#define RX_MATH_CONSTANTS_H

namespace rx::math {

template<typename T>
inline constexpr const T k_pi{3.14159265358979323846264338327950288};

template<typename T>
inline constexpr const T k_tau{k_pi<T>*2.0};

} // namespace rx

#endif //
