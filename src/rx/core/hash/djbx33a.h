#ifndef RX_CORE_HASH_DJBX33A_H
#define RX_CORE_HASH_DJBX33A_H
#include "rx/core/array.h"

namespace Rx::Hash {

RX_API Array<Byte[16]> djbx33a(const Byte* _data, Size _size);

} // namespace Rx::Hash

#endif // RX_CORE_HASH_DJBX33A_H
