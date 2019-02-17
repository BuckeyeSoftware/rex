#ifndef RX_MATH_HALF_H
#define RX_MATH_HALF_H

#include <rx/core/types.h> // rx_u16

namespace rx::math {

struct half {
  constexpr half(const half& h);

  half(rx_f32 f);
  half(rx_f64 f);

  rx_f32 to_f32() const;
  rx_f64 to_f64() const;

  friend half operator+(half lhs, half rhs);
  friend half operator-(half lhs, half rhs);
  friend half operator*(half lhs, half rhs);
  friend half operator/(half lhs, half rhs);

  friend half& operator+=(half &lhs, half rhs);
  friend half& operator-=(half &lhs, half rhs);
  friend half& operator*=(half &lhs, half rhs);
  friend half& operator/=(half &lhs, half rhs);

  friend half operator-(half h);
  friend half operator+(half h);

private:
  constexpr explicit half(rx_u16 bits);

  half to_half(rx_f32 f);
  rx_u16 m_bits;
};

inline constexpr half::half(const half& h)
  : m_bits{h.m_bits}
{
}

inline half::half(rx_f32 f)
  : half{to_half(f)}
{
}

inline half::half(rx_f64 f)
  : half{static_cast<rx_f32>(f)}
{
}

inline rx_f64 half::to_f64() const {
  return static_cast<rx_f64>(to_f32());
}

inline half operator+(half lhs, half rhs) {
  return lhs.to_f32() + rhs.to_f32();
}

inline half operator-(half lhs, half rhs) {
  return lhs.to_f32() - rhs.to_f32();
}

inline half operator*(half lhs, half rhs) {
  return lhs.to_f32() * rhs.to_f32();
}

inline half operator/(half lhs, half rhs) {
  return lhs.to_f32() / rhs.to_f32();
}

inline half& operator+=(half &lhs, half rhs) {
  return lhs = lhs + rhs;
}

inline half& operator-=(half &lhs, half rhs) {
  return lhs = lhs - rhs;
}

inline half& operator*=(half &lhs, half rhs) {
  return lhs = lhs * rhs;
}

inline half& operator/=(half &lhs, half rhs) {
  return lhs = lhs / rhs;
}

inline half operator-(half h) {
  return -h.to_f32();
}

inline half operator+(half h) {
  return +h.to_f32();
}

inline constexpr half::half(rx_u16 bits)
  : m_bits{bits}
{
}

} // namespace rx::math

#endif // RX_MATH_HALF_H
