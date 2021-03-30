#ifndef RX_CORE_STREAM_OPERATIONS_H
#define RX_CORE_STREAM_OPERATIONS_H
#include "rx/core/types.h"

namespace Rx::Stream {

struct Stat {
  Uint64 size;
  // Can extend with aditional stats.
};

// Stream flags.
enum : Uint32 {
  READ  = 1 << 0,
  WRITE = 1 << 1,
  STAT  = 1 << 3,
  FLUSH = 1 << 4
};

enum class Whence : Uint8 {
  SET,     // Beginning of stream.
  CURRENT, // Current position
  END      // End of stream.
};

} // namespace Rx::Stream

#endif // RX_CORE_STREAM_OPERATIONS_H