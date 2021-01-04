#ifndef RX_CORE_MATH_HALF_H
#define RX_CORE_MATH_HALF_H
#include "rx/core/types.h"
#include "rx/core/assert.h" // RX_ASSERT

namespace Rx::Math {

struct RX_API Half {
  constexpr Half();
  constexpr Half(const Half&) = default;
  constexpr Half& operator=(const Half&) = default;

  Half(Float32 _f);

  Float32 to_f32() const;
  Float64 to_f64() const;

  friend Half operator+(Half _lhs, Half _rhs);
  friend Half operator-(Half _lhs, Half _rhs);
  friend Half operator*(Half _lhs, Half _rhs);
  friend Half operator/(Half _lhs, Half _rhs);

  friend Half& operator+=(Half& _lhs, Half _rhs);
  friend Half& operator-=(Half& _lhs, Half _rhs);
  friend Half& operator*=(Half& _lhs, Half _rhs);
  friend Half& operator/=(Half& _lhs, Half _rhs);

  friend Half operator-(Half _h);
  friend Half operator+(Half _h);

private:
  struct Bits { Uint16 bits; };
  constexpr explicit Half(Bits _bits);

  Half to_half(Float32 _f);

  Uint16 m_bits;
};

inline constexpr Half::Half()
  : m_bits{0}
{
}

inline Half::Half(Float32 _f)
  : Half{to_half(_f)}
{
}

inline Float64 Half::to_f64() const {
  return static_cast<Float64>(to_f32());
}

inline Half operator+(Half _lhs, Half _rhs) {
  return _lhs.to_f32() + _rhs.to_f32();
}

inline Half operator-(Half _lhs, Half _rhs) {
  return _lhs.to_f32() - _rhs.to_f32();
}

inline Half operator*(Half _lhs, Half _rhs) {
  return _lhs.to_f32() * _rhs.to_f32();
}

inline Half operator/(Half _lhs, Half _rhs) {
  return _lhs.to_f32() / _rhs.to_f32();
}

inline Half& operator+=(Half& _lhs, Half _rhs) {
  return _lhs = _lhs + _rhs;
}

inline Half& operator-=(Half& _lhs, Half _rhs) {
  return _lhs = _lhs - _rhs;
}

inline Half& operator*=(Half& _lhs, Half _rhs) {
  return _lhs = _lhs * _rhs;
}

inline Half& operator/=(Half& _lhs, Half _rhs) {
  return _lhs = _lhs / _rhs;
}

inline Half operator-(Half _h) {
  return -_h.to_f32();
}

inline Half operator+(Half _h) {
  return +_h.to_f32();
}

inline constexpr Half::Half(Bits _bits)
  : m_bits{_bits.bits}
{
}

} // namespace Rx::Math

#endif // RX_CORE_MATH_HALF_H
