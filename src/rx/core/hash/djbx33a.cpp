#include <string.h>
#include "rx/core/hash/djbx33a.h"

namespace Rx::Hash {

Array<Byte[16]> djbx33a(const Byte* _data, Size _size) {
  Uint32 state[4] = { 5381, 5381, 5381, 5381 };
  Uint32 s = 0;
  for (auto end = _data + _size, p = _data; p < end; p++) {
    state[s] = state[s] * 33 + *p;
    s = (s + 1) & 3;
  }

  Array<Byte[16]> result;
  memcpy(result.data(), state, sizeof state);
  return result;
}

} // namespace Rx::Hash
