#ifndef RX_CORE_CONCEPTS_NO_MOVE_ASSIGN_H
#define RX_CORE_CONCEPTS_NO_MOVE_ASSIGN_H

namespace Rx::Concepts {

struct NoMoveAssign {
  constexpr NoMoveAssign() = default;
  constexpr NoMoveAssign& operator=(NoMoveAssign&&) = delete;
};

} // namespace Rx::Concepts

#endif // RX_CORE_CONCEPTS_NO_MOVE_ASSIGN_H
