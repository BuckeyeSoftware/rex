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
  // Out of range.
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

bool MemoryStream::on_truncate(Uint64 _size) {
  // Truncation would actually expand
  if (_size > m_capacity) {
    return false;
  }

  m_size = _size;

  return true;
}

Uint64 MemoryStream::on_zero(Uint64 _size, Uint64 _offset) {
  // Out of bounds.
  if (_offset >= m_size) {
    return 0;
  }

  // Don't allow over zeroring.
  const auto wr = Algorithm::min(_size, m_size - _offset);

  memset(m_data + _offset, 0, wr);

  return wr;
}

Uint64 MemoryStream::on_copy(Uint64 _dst_offset, Uint64 _src_offset, Uint64 _size) {
  // Out of range.
  if (_dst_offset >= m_capacity || _src_offset >= m_capacity) {
    return 0;
  }

  // Don't allow over reading.
  const auto rd_bytes = Algorithm::min(_size, m_size - _src_offset);

  // Don't allow over writing.
  const auto wr_bytes = Algorithm::min(rd_bytes, m_capacity - _dst_offset);

  // The write would expand the size.
  if (_dst_offset > m_size) {
    // Zero the contents in range [m_size, _dst_offset).
    memset(m_data + m_size, 0, _dst_offset - m_size);
    m_size = _dst_offset + wr_bytes;
  }

  // NOTE(dweiler): The interface contract of |on_copy| must not overlap!
  memcpy(m_data + _src_offset, m_data + _dst_offset, wr_bytes);

  return wr_bytes;
}

} // namespace Rx::Stream