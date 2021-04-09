#include <limits.h> // UCHAR_MAX
#include <string.h> // memmove

#include "rx/core/stream/context.h"

#include "rx/core/string.h"
#include "rx/core/abort.h"

#include "rx/core/hints/may_alias.h"

namespace Rx::Stream {

static Optional<LinearBuffer> convert_text_encoding(LinearBuffer&& data_) {
  // Ensure the data contains a null-terminator.
  if (data_.last() != 0 && !data_.push_back(0)) {
    return nullopt;
  }

  const bool utf16_le = data_.size() >= 2 && data_[0] == 0xFF && data_[1] == 0xFE;
  const bool utf16_be = data_.size() >= 2 && data_[0] == 0xFE && data_[1] == 0xFF;

  // UTF-16.
  if (utf16_le || utf16_be) {
    // Remove the BOM.
    data_.erase(0, 2);

    auto contents = reinterpret_cast<Uint16*>(data_.data());
    const Size chars = data_.size() / 2;
    if (utf16_be) {
      // Swap the bytes around in the contents to convert BE to LE.
      for (Size i = 0; i < chars; i++) {
        contents[i] = (contents[i] >> 8) | (contents[i] << 8);
      }
    }

    // Determine how many bytes are needed to convert the encoding.
    const Size length = utf16_to_utf8(contents, chars, nullptr);

    // Convert UTF-16 to UTF-8.
    LinearBuffer result{data_.allocator()};
    if (!result.resize(length)) {
      return nullopt;
    }
    utf16_to_utf8(contents, chars, reinterpret_cast<char*>(result.data()));
    return result;
  } else if (data_.size() >= 3 && data_[0] == 0xEF && data_[1] == 0xBB && data_[2] == 0xBF) {
    // Remove the BOM.
    data_.erase(0, 3);
  }

  return Utility::move(data_);
}

Uint64 Context::on_read(Byte*, Uint64, Uint64) {
  abort("stream does not implement on_read");
}

Uint64 Context::on_write(const Byte*, Uint64, Uint64) {
  abort("stream does not implement on_write");
}

bool Context::on_stat(Stat&) const {
  abort("stream does not implement on_stat");
}

bool Context::on_flush() {
  abort("stream does not implement on_flush");
}

Uint64 Context::read(Byte* _data, Uint64 _size) {
  if (!(m_flags & READ) || _size == 0) {
    return 0;
  }

  const auto read = on_read(_data, _size, m_offset);
  if (read != _size) {
    m_flags |= EOS;
  }

  m_offset += read;
  return read;
}

Uint64 Context::write(const Byte* _data, Uint64 _size) {
  if (!(m_flags & WRITE) || _size == 0) {
    return 0;
  }

  const auto write = on_write(_data, _size, m_offset);
  m_offset += write;
  return write;
}

bool Context::seek(Sint64 _where, Whence _whence) {
  // Calculate the new offset based on |_whence|.
  if (_whence == Whence::CURRENT) {
    if (m_flags & EOS) {
      if (_where < 0) {
        // Rewinds the stream. Unset EOS.
        m_flags &= ~EOS;
      } else if (_where > 0) {
        // Would seek beyond the end of the stream.
        return false;
      }
    }
    m_offset = m_offset + _where;
  } else if (_whence == Whence::SET) {
    // Seeking to negative position is an error.
    if (_where < 0) {
      return false;
    }

    const auto where = static_cast<Uint64>(_where);

    if (m_flags & EOS) {
      if (where < m_offset) {
        // Rewinds the stream. Unset EOS.
        m_flags &= ~EOS;
      } else if (where > m_offset) {
        // Would seek beyond the end of the stream.
        return false;
      }
    }
    m_offset = _where;
  } else if (_whence == Whence::END) {
    // Seeking past the end of the stream is an error.
    if (_where > 0) {
      return false;
    }

    // Can only seek relative to end if stat is supported.
    auto s = stat();
    if (!s) {
      return false;
    }

    m_offset = s->size + _where;

    if (_where < 0) {
      m_flags &= ~EOS;
    } else {
      m_flags |= EOS;
    }
  }

  return true;
}

void Context::rewind() {
  m_flags &= ~EOS;
  m_offset = 0;
}

Optional<Stat> Context::stat() const {
  // Stream does not support stating.
  if (!(m_flags & STAT)) {
    return nullopt;
  }

  if (Stat s; on_stat(s)) {
    return s;
  }

  // Stream supports stating but |on_stat| failed for some reason.
  return nullopt;
}

bool Context::flush() {
  // Stream does not support flushing.
  if (!(m_flags & FLUSH)) {
    return false;
  }

  return on_flush();
}

Optional<LinearBuffer> Context::read_binary(Memory::Allocator& _allocator) {
  LinearBuffer result{_allocator};

  auto n_bytes = size();
  if (n_bytes && *n_bytes) {
    if (!result.resize(*n_bytes)) {
      return nullopt;
    }

    if (read(result.data(), *n_bytes) != *n_bytes) {
      return nullopt;
    }
  } else {
    // Stream does not support quering file size. Read in a loop until EOS.
    while (!is_eos()) {
      Byte data[4096];
      auto bytes = read(data, sizeof data);
      if (!result.append(data, bytes)) {
        return nullopt;
      }
    }
  }

  return result;
}

Optional<LinearBuffer> Context::read_text(Memory::Allocator& _allocator) {
  if (auto result = read_binary(_allocator)) {
    // Convert the given byte stream into a compatible UTF-8 encoding. This will
    // introduce a null-terminator, strip Unicode BOMs and convert UTF-16
    // encodings to UTF-8.
    auto data = convert_text_encoding(Utility::move(*result));
    if (!data) {
      return nullopt;
    }

#if defined(RX_PLATFORM_WINDOWS)
    // Quickly scan through word at a time |_src| for CR.
    auto scan = [](const void* _src, Size _size) {
      static constexpr auto SS = sizeof(Size);
      static constexpr auto ALIGN = SS - 1;
      static constexpr auto ONES = -1_z / UCHAR_MAX; // All bits set.
      static constexpr auto HIGHS = ONES * (UCHAR_MAX / 2 + 1); // All high bits set.
      static constexpr auto C = static_cast<const Byte>('\r');
      static constexpr auto K = ONES * C;

      auto has_zero = [&](Size _value) {
        return _value - ONES & (~_value) & HIGHS;
      };

      // Search for CR until |s| is aligned on |ALIGN| alignment.
      auto s = reinterpret_cast<const Byte*>(_src);
      auto n = _size;
      for (; (reinterpret_cast<UintPtr>(s) & ALIGN) && n && *s != C; s++, n--);

      // Do checks for CR word at a time, stopping at word containing CR.
      if (n && *s != C) {
        // Need to typedef with an alias type since we're breaking strict
        // aliasing, let the compiler know.
        typedef Size RX_HINT_MAY_ALIAS WordType;

        // Scan word at a time, stopping at word containing first |C|.
        auto w = reinterpret_cast<const WordType*>(s);
        for (; n >= SS && !has_zero(*w ^ K); w++, n -= SS);
        s = reinterpret_cast<const Byte*>(w);
      }

      // Handle trailing bytes to determine where in word |C| is.
      for (; n && *s != C; s++, n--);

      return n ? s : nullptr;
    };

    const Byte* src = data->data();
    Byte* dst = data->data();
    Size size = data->size();
    auto next = scan(src, size);
    if (!next) {
      // Contents do not contain any CR.
      return data;
    }

    // Remove all instances of CR from the byte stream using instruction-level
    // parallelism.
    //
    // Note that the very first iteration will always have |src| == |dst|, so
    // optimize away the initial memmove here as nothing needs to be moved at
    // the beginning.
    const auto length = next - src;
    const auto length_plus_one = length + 1;
    dst += length;
    src += length_plus_one;
    size -= length_plus_one;

    // Subsequent iterations will move contents forward.
    while ((next = scan(src, size))) {
      const auto length = next - src;
      const auto length_plus_one = length + 1;
      memmove(dst, src, length);
      dst += length;
      src += length_plus_one;
      size -= length_plus_one;
    }

    // Ensure the result is null-terminated after all those moves.
    *dst++ = '\0';

    // Respecify the size of storage after removing all those CRs.
    data->resize(dst - data->data());
#endif
    return data;
  }

  return nullopt;
}

} // namespace Rx
