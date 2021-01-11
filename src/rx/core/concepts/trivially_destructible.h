#ifndef RX_CORE_CONCEPTS_TRIVIALLY_DESTRUCTIBLE_H
#define RX_CORE_CONCEPTS_TRIVIALLY_DESTRUCTIBLE_H

namespace Rx::Concepts {

template<typename T>
concept TriviallyDestructible = __has_trivial_destructor(T);

} // namespace Rx::Concepts

#endif // RX_CORE_CONCEPTS_TRIVIALLY_DESTRUCTIBLE_H
