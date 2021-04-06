#ifndef RX_CORE_STREAM_OPERATIONS_H
#define RX_CORE_STREAM_OPERATIONS_H
#include "rx/core/types.h"

/// \file operations.h

namespace Rx::Stream {

/// \brief Statistics of a stream.
struct Stat {
  Uint64 size; ///< Size in bytes.
  // Can extend with aditional stats.
};

/// \brief Stream flags.
/// OR'd bitmask of these flags indicate what features a Context supports.
enum : Uint32 {
  READ  = 1 << 0,
  WRITE = 1 << 1,
  STAT  = 1 << 3,
  FLUSH = 1 << 4
};

/// \brief Relative location for stream seeks.
enum class Whence : Uint8 {
  SET,     ///< Beginning of stream.
  CURRENT, ///< Current position
  END      ///< End of stream.
};

} // namespace Rx::Stream

#endif // RX_CORE_STREAM_OPERATIONS_H