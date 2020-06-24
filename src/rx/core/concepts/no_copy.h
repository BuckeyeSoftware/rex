#ifndef RX_CORE_CONCEPTS_NO_COPY_H
#define RX_CORE_CONCEPTS_NO_COPY_H
#include "rx/core/hints/empty_bases.h"

#include "rx/core/concepts/no_copy_assign.h"
#include "rx/core/concepts/no_copy_construct.h"

namespace Rx::Concepts {

struct RX_HINT_EMPTY_BASES NoCopy
  : NoCopyAssign
  , NoCopyConstruct
{
  constexpr NoCopy() = default;
};

} // namespace rx::concepts

#endif // RX_CORE_CONCEPTS_NO_COPY_H
