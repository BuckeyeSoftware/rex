#ifndef RX_MATH_RANGE_H
#define RX_MATH_RANGE_H

namespace Rx::Math {

template<typename T>
struct Range {
  constexpr Range(const T& _min, const T& _max);
  T min;
  T max;
};

template<typename T>
constexpr Range<T>::Range(const T& _min, const T& _max)
  : min{_min}
  , max{_max}
{
}

} // namespace Rx::Math

#endif // RX_MATH_RANGE_H
