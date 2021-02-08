#ifndef RX_CORE_CONCEPTS_COPYABLE_H
#define RX_CORE_CONCEPTS_COPYABLE_H

namespace Rx::Concepts {

template<typename T>
concept Copyable = requires(const T& _value) {
  { T::copy(_value) };
};

} // namespace Rx::Concepts

#endif // RX_CORE_CONCEPTS_COPYABLE_H