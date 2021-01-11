#ifndef RX_CORE_CONCEPTS_TRIVIALLY_COPYABLE_H
#define RX_CORE_CONCEPTS_TRIVIALLY_COPYABLE_H

namespace Rx::Concepts {

template<typename T>
concept TriviallyCopyable = __is_trivially_copyable(T);

} // namespace Rx::Concepts

#endif // RX_CORE_CONCEPTS_TRIVIALLY_COPYABLE_H
