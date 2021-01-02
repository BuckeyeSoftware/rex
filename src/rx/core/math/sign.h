#ifndef RX_CORE_MATH_SIGN_H
#define RX_CORE_MATH_SIGN_H

namespace Rx::Math {

template<typename T>
constexpr T sign(T _value) {
    return T(T{0} < _value) - T(_value < T{0});
}

} // namespace Rx::Math

#endif // RX_CORE_MATH_SIGN_H
