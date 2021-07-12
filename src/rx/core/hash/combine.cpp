#include "rx/core/hash/combine.h"
#include "rx/core/utility/wire.h"

namespace Rx::Hash {

Array<Byte[16]> combine(const Array<Byte[16]>& _hash1, const Array<Byte[16]>& _hash2) {
  Array<Byte[16]> result;
  Utility::write_u32(result.data() + 0, combine_u32(Utility::read_u32(_hash1.data() + 0), Utility::read_u32(_hash2.data() + 0)));
  Utility::write_u32(result.data() + 4, combine_u32(Utility::read_u32(_hash1.data() + 4), Utility::read_u32(_hash2.data() + 4)));
  Utility::write_u32(result.data() + 8, combine_u32(Utility::read_u32(_hash1.data() + 8), Utility::read_u32(_hash2.data() + 8)));
  Utility::write_u32(result.data() + 12, combine_u32(Utility::read_u32(_hash1.data() + 12), Utility::read_u32(_hash2.data() + 12)));
  return result;
}

} // namespace Rx::Hash