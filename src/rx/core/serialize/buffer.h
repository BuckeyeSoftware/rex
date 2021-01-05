#ifndef RX_CORE_SERIALIZE_BUFFER_H
#define RX_CORE_SERIALIZE_BUFFER_H
#include "rx/core/types.h"

namespace Rx {
  struct Stream;
}

namespace Rx::serialize {

struct RX_API Buffer {
  static inline constexpr Size SIZE = 4096;

  enum class Mode : Uint8 {
    READ,
    WRITE
  };

  Buffer(Stream* _stream, Mode _mode);
  ~Buffer();

  [[nodiscard]] bool write_byte(Byte _byte);
  [[nodiscard]] bool write_bytes(const Byte* _bytes, Size _size);

  [[nodiscard]] bool read_byte(Byte* byte_);
  [[nodiscard]] bool read_bytes(Byte* bytes_, Size _size);

  [[nodiscard]] bool read(Uint64 _at_most = SIZE);
  [[nodiscard]] bool flush();

private:
  Stream* m_stream;
  Mode m_mode;

  Byte m_buffer[SIZE];
  Size m_cursor;
  Size m_length;
};

} // namespace Rx::serialize

#endif // RX_CORE_SERIALIZE_BUFFER_H
