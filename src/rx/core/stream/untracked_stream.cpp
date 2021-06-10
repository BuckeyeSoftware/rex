#include <limits.h> // UCHAR_MAX
#include <string.h> // memmove

#include "rx/core/algorithm/min.h"

#include "rx/core/stream/untracked_stream.h"

#include "rx/core/string.h"
#include "rx/core/abort.h"

#include "rx/core/hints/may_alias.h"

namespace Rx::Stream {

Uint64 UntrackedStream::on_read(Byte*, Uint64, Uint64) {
  abort("Stream does not implement on_read");
}

Uint64 UntrackedStream::on_write(const Byte*, Uint64, Uint64) {
  abort("Stream does not implement on_write");
}

Optional<Stat> UntrackedStream::on_stat() const {
  abort("Stream does not implement on_stat");
}

bool UntrackedStream::on_flush() {
  abort("Stream does not implement on_flush");
}

bool UntrackedStream::on_truncate(Uint64) {
  abort("Stream does not implement on_truncate");
}

Uint64 UntrackedStream::on_zero(Uint64 _size, Uint64 _offset) {
  // 4 KiB of zeros written in a loop.
  Byte zero[4096] = {0};
  Uint64 bytes = 0;
  while (bytes < _size) {
    // Which ever is smaller is the max bytes to zero.
    const auto remain = _size - bytes;
    const auto max = Algorithm::min(remain, Uint64(sizeof zero));
    if (const auto wr = on_write(zero, max, _offset + bytes)) {
      bytes += wr;
      if (wr != max) {
        // Out of stream space.
        break;
      }
    } else {
      // End of stream.
    }
  }
  return bytes;
}

Uint64 UntrackedStream::on_copy(Uint64 _dst_offset, Uint64 _src_offset, Uint64 _size) {
  // 4 KiB copies from one place to another.
  Byte buffer[4096];
  Uint64 bytes = 0;
  while (bytes < _size) {
    // Which ever is smaller is the max bytes to copy.
    const auto remain = _size - bytes;
    const auto max = Algorithm::min(remain, Uint64(sizeof buffer));
    if (const auto rd = on_read(buffer, max, _src_offset + bytes)) {
      const auto wr = on_write(buffer, rd, _dst_offset + bytes);
      bytes += wr;
      if (wr != rd) {
        // Out of stream space.
        break;
      }
    } else {
      // End of stream.
      break;
    }
  }
  return bytes;
}

static Optional<LinearBuffer> convert_text_encoding(LinearBuffer&& data_) {
  // Ensure the data contains a null-terminator.
  if ((data_.is_empty() || data_.last() != 0) && !data_.push_back(0)) {
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

Optional<LinearBuffer> UntrackedStream::read_binary(Memory::Allocator& _allocator) {
  LinearBuffer result{_allocator};

  // When stat is supported try and use it to optimize the read.
  if (m_flags & STAT) {
    if (auto stat = on_stat(); !result.resize(stat->size)) {
      // Out of memory.
      return nullopt;
    }
  }

  if (result.is_empty()) {
    // Couldn't determine size of the stream. This could be because STAT
    // is not supported or because |on_stat| returned a zero size. Read the
    // contents from the stream in a loop until EOS.
    Byte buffer[4096];
    Uint64 offset = 0;
    for (;;) {
      const auto read = on_read(buffer, sizeof buffer, offset);
      if (read == 0) {
        // End of stream.
        break;
      }
      if (!result.append(buffer, read)) {
        // Out of memory.
        return nullopt;
      }
      offset += read;
    }
  } else if (on_read(result.data(), result.size(), 0) != result.size()) {
    // Couldn't read all the contents.
    return nullopt;
  }

  return result;
}

Optional<LinearBuffer> UntrackedStream::read_text(Memory::Allocator& _allocator) {
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
    (void)data->resize(dst - data->data());
#endif
    return data;
  }

  return nullopt;
}

} // namespace Rx
