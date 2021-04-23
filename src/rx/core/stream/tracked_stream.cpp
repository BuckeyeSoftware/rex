#include "rx/core/stream/tracked_stream.h"
#include "rx/core/stream/untracked_stream.h"

namespace Rx::Stream {

Uint64 TrackedStream::read(Byte* _data, Uint64 _size) {
  if (!(m_stream.flags() & READ)) {
    // Stream does not support reading.
    return 0;
  }

  if (_size == 0) {
    // Zero-size reads always succeed.
    return 0;
  }

  const auto read = m_stream.on_read(_data, _size, m_offset);
  if (read != _size) {
    m_is_eos = true;
  }

  m_offset += read;
  return read;
}

Uint64 TrackedStream::write(const Byte* _data, Uint64 _size) {
  if (!(m_stream.flags() & WRITE)) {
    // Stream does not support writing.
    return 0;
  }

  if (_size == 0) {
    // Zero-size writes always succeed.
    return 0;
  }

  const auto write = m_stream.on_write(_data, _size, m_offset);
  m_offset += write;
  return write;
}

bool TrackedStream::seek(Sint64 _where, Whence _whence) {
  // Calculate the new offset based on |_whence|.
  if (_whence == Whence::CURRENT) {
    if (m_is_eos) {
      if (_where < 0) {
        // Rewinds the stream. Unset EOS.
        m_is_eos = false;
      } else if (_where > 0) {
        // Would seek beyond the end of the stream.
        return false;
      }
    }
    m_offset = m_offset + _where;
  } else if (_whence == Whence::SET) {
    if (_where < 0) {
      // Seeking to negative position is an error.
      return false;
    }

    const auto where = static_cast<Uint64>(_where);

    if (m_is_eos) {
      if (where < m_offset) {
        // Rewinds the stream. Unset EOS.
        m_is_eos = false;
      } else if (where > m_offset) {
        // Would seek beyond the end of the stream.
        return false;
      }
    }
    m_offset = _where;
  } else if (_whence == Whence::END) {
    if (_where > 0) {
      // Seeking past the end of the stream is an error.
      return false;
    }

    auto s = stat();
    if (!s) {
      // Stat not supported.
      return false;
    }

    // The underlying stream may report a zero size. In this case Whence::END
    // behaves as seeking to |_where| from the beginning of the file. This
    // is consistent behavior with fseek on non-seekable files.
    m_offset = s->size + _where;

    m_is_eos = _where >= 0;
  }

  return true;
}

void TrackedStream::rewind() {
  m_is_eos = false;
  m_offset = 0;
}

Optional<Stat> TrackedStream::stat() const {
  if (!(m_stream.flags() & STAT)) {
    // Stream does not support stating.
    return nullopt;
  }
  return m_stream.on_stat();
}

bool TrackedStream::flush() {
  if (!(m_stream.flags() & FLUSH)) {
    // Stream does not support flushing.
    return false;
  }
  return m_stream.on_flush();
}

bool TrackedStream::truncate(Uint64 _size) {
  if (!(m_stream.flags() & TRUNCATE)) {
    // Stream does not support truncating.
    return false;
  }
  return m_stream.on_truncate(_size);
}

Uint64 TrackedStream::flags() const {
  return m_stream.flags();
}

} // namespace Rx::Stream