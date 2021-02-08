#ifndef RX_CORE_CONCEPTS_TRIVIAL_H
#define RX_CORE_CONCEPTS_TRIVIAL_H

namespace Rx::Concepts {

template<typename T>
concept Trivial = __is_trivial(T);

} // namespace Rx::Concepts

#endif // RX_CORE_CONCEPTS_TRIVIAL_H