#ifndef RX_CORE_CONCEPTS_ENUM_H
#define RX_CORE_CONCEPTS_ENUM_H

namespace Rx::Concepts {

template<typename T>
concept Enum = __is_enum(T);

} // namespace Rx::Concepts

#endif // RX_CORE_CONCEPTS_ENUM_H
