#ifndef RX_CORE_CONCEPTS_NO_MOVE_H
#define RX_CORE_CONCEPTS_NO_MOVE_H
#include "rx/core/hints/empty_bases.h"

#include "rx/core/concepts/no_move_assign.h"
#include "rx/core/concepts/no_move_construct.h"

namespace Rx::Concepts {

struct RX_HINT_EMPTY_BASES NoMove
  : NoMoveAssign
  , NoMoveConstruct
{
  constexpr NoMove() = default;
};

} // namespace rx::concepts

#endif // RX_CORE_CONCEPTS_NO_MOVE_H
