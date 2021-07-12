#include "rx/core/hash/hasher.h"
#include "rx/core/hash/combine.h"

#include "rx/core/utility/wire.h"

namespace Rx::Hash {

Size Hasher<Array<Byte[16]>>::operator()(const Array<Byte[16]>& _value) const {
  const auto word0 = Utility::read_u32(_value.data() + 0);
  const auto word1 = Utility::read_u32(_value.data() + 4);
  const auto word2 = Utility::read_u32(_value.data() + 8);
  const auto word3 = Utility::read_u32(_value.data() + 12);
  return combine_u32(combine_u32(word0, word1), combine_u32(word2, word3));
}

} // namespace Rx::Hash