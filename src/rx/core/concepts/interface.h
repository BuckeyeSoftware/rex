#ifndef RX_CORE_CONCEPTS_INTERFACE_H
#define RX_CORE_CONCEPTS_INTERFACE_H
#include "rx/core/concepts/no_copy.h"
#include "rx/core/concepts/no_move.h"

namespace Rx::Concepts {

struct RX_HINT_EMPTY_BASES Interface
  : NoCopy
  , NoMove
{
  constexpr Interface() = default;
  virtual ~Interface() = 0;
};

inline Interface::~Interface() {
  // { empty }
}

} // namespace rx::concepts

#endif // RX_CORE_CONCEPTS_INTERFACE_H
