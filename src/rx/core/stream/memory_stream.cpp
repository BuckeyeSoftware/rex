#include <string.h> // memcpy

#include "rx/core/stream/memory_stream.h"

#include "rx/core/algorithm/min.h"
#include "rx/core/algorithm/max.h"

namespace Rx::Stream {

Uint64 MemoryStream::on_read(Byte* data_, Uint64 _size, Uint64 _offset) {
  if (_offset >= m_size) {
    return 0;
  }
  const auto bytes = Algorithm::min(_size, m_size - _offset);
  memcpy(data_, m_data + _offset, _size);
  return bytes;
}

Uint64 MemoryStream::on_write(const Byte* _data, Uint64 _size, Uint64 _offset) {
  if (_offset >= m_capacity) {
    return 0;
  }

  const auto bytes = Algorithm::min(_size, m_capacity - _offset);

  // The write would expand the size.
  if (_offset > m_size) {
    // Zero the contents in range [m_size, _offset).
    memset(m_data + m_size, 0, _offset - m_size);
    m_size = _offset + bytes;
  }

  memcpy(m_data + _offset, _data, _size);

  return bytes;
}

bool MemoryStream::on_stat(Stat& stat_) const {
  stat_.size = m_size;
  return true;
}

} // namespace Rx::Stream