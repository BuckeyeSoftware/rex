#ifndef RX_CORE_CONCEPTS_NO_COPY_ASSIGN_H
#define RX_CORE_CONCEPTS_NO_COPY_ASSIGN_H

namespace Rx::Concepts {

struct NoCopyAssign {
  constexpr NoCopyAssign() = default;
  constexpr NoCopyAssign& operator=(const NoCopyAssign&) = delete;
};

} // namespace Rx::Concepts

#endif // RX_CORE_CONCEPTS_NO_COPY_ASSIGN_H
