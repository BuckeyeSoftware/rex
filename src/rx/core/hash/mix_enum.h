#ifndef RX_CORE_HASH_MIX_ENUM_H
#define RX_CORE_HASH_MIX_ENUM_H
#include "rx/core/hash/mix_int.h"
#include "rx/core/concepts/enum.h"
#include "rx/core/traits/underlying_type.h"

namespace Rx::Hash {

template<Concepts::Enum E>
inline constexpr Size mix_enum(E _value) {
  return mix_int(static_cast<Traits::UnderlyingType<E>>(_value));
}

} // namespace Rx::Hash

#endif // RX_CORE_HASH_MIX_ENUM_H
