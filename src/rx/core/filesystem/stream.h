#ifndef RX_CORE_FILESYSTEM_STREAM_H
#define RX_CORE_FILESYSTEM_STREAM_H

#include "rx/core/types.h"
#include "rx/core/concepts/interface.h"

namespace rx::filesystem {

struct stream
  : concepts::interface
{
  // Read |_size| bytes from stream into |_data|.
  virtual rx_u64 read(rx_byte* _data, rx_u64 _size) = 0;

  // Rrite |_size| bytes from |_data| into stream.
  virtual rx_u64 write(const rx_byte* _data, rx_u64 _size) = 0;

  // Seek to |where| in stream.
  virtual bool seek(rx_u64 _where) = 0;

  // Flush any buffered contents in the stream out.
  virtual bool flush() = 0;
};

} // namespace rx::filesystem

#endif // RX_CORE_FILESYSTEM_STREAM_H
