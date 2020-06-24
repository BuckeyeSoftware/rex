#ifndef RX_CORE_CONCEPTS_NO_COPY_CONSTRUCT_H
#define RX_CORE_CONCEPTS_NO_COPY_CONSTRUCT_H

namespace Rx::Concepts {

struct NoCopyConstruct {
  constexpr NoCopyConstruct() = default;
  constexpr NoCopyConstruct(const NoCopyConstruct&) = delete;
};

} // namespace Rx::Concepts

#endif // RX_CORE_CONCEPTS_NO_COPY_CONSTRUCT_H
