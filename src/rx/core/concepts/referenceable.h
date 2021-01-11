#ifndef RX_CORE_CONCEPTS_REFERENCEABLE_H
#define RX_CORE_CONCEPTS_REFERENCEABLE_H

namespace Rx::Concepts {

template<typename T>
concept Referenceable = requires(T& value) {
  value;
};

} // namespace Rx::Concepts

#endif // RX_CORE_CONCEPTS_REFERENCEABLE_H
