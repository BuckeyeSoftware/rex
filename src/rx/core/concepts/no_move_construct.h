#ifndef RX_CORE_CONCEPTS_NO_MOVE_CONSTRUCT_H
#define RX_CORE_CONCEPTS_NO_MOVE_CONSTRUCT_H

namespace Rx::Concepts {

struct NoMoveConstruct {
  constexpr NoMoveConstruct() = default;
  constexpr NoMoveConstruct(NoMoveConstruct&&) = delete;
};

} // namespace Rx::Concepts

#endif // RX_CORE_CONCEPTS_NO_MOVE_CONSTRUCT_H
