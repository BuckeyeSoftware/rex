#include <string.h> // memcmp

#include "rx/core/serialize/decoder.h"
#include "rx/core/linear_buffer.h"
#include "rx/core/assert.h"

#include "rx/core/stream/context.h"

namespace Rx::serialize {

Decoder::Decoder(Memory::Allocator& _allocator, Stream::Context& _stream)
  : m_allocator{_allocator}
  , m_stream{&_stream}
  , m_buffer{_stream, Buffer::Mode::READ}
  , m_message{allocator()}
  , m_strings{allocator()}
{
  // Read header and strings.
  RX_ASSERT(read_header(), "failed to read header");
  RX_ASSERT(read_strings(), "failed to read strings");

  // Read data into |m_buffer| for the decoder to begin using.
  RX_ASSERT(m_buffer.read(m_header.data_size), "buffer failed");
}

Decoder::~Decoder() {
  RX_ASSERT(finalize(), "finalization failed");
}

bool Decoder::read_uint(Uint64& value_) {
  Byte byte;

  auto shift = 0_u64;
  auto value = 0_u64;

  do if (!m_buffer.read_byte(&byte)) {
    return error("unexpected end of stream");
  } else {
    const Uint64 slice = byte & 0x7f;
    if (shift >= 64 || slice << shift >> shift != slice) {
      return error("ULEB128 value too large");
    }
    value += static_cast<Uint64>(slice) << shift;
    shift += 7;
  } while (byte >= 0x80);

  value_ = value;

  return true;
}

bool Decoder::read_sint(Sint64& value_) {
  Byte byte;

  auto shift = 0_u64;
  auto value = 0_u64;

  do if (!m_buffer.read_byte(&byte)) {
    return error("unexpected end of stream");
  } else {
    value |= (static_cast<Uint64>(byte & 0x7f) << shift);
    shift += 7;
  } while (byte >= 0x80);

  // Sign extend negative numbers.
  if (shift < 64 && (byte & 0x40)) {
    value |= -1_u64 << shift;
  }

  value_ = static_cast<Sint64>(value);

  return true;
}

bool Decoder::read_float(Float32& value_) {
  return m_buffer.read_bytes(reinterpret_cast<Byte*>(&value_), sizeof value_);
}

bool Decoder::read_bool(bool& value_) {
  Byte byte;
  if (!m_buffer.read_byte(&byte)) {
    return false;
  }

  if (byte != 0 && byte != 1) {
    return error("encoding error");
  }

  value_ = byte ? true : false;

  return true;
}

bool Decoder::read_byte(Byte& byte_) {
  return m_buffer.read_byte(&byte_);
}

bool Decoder::read_string(String& result_) {
  Uint64 index = 0;
  if (!read_uint(index)) {
    return false;
  }

  result_ = m_strings[index];

  return true;
}

bool Decoder::read_float_array(Float32* result_, Size _count) {
  Uint64 count = 0;
  if (!read_uint(count)) {
    return false;
  }

  if (count != _count) {
    return error("array count mismatch");
  }

  for (Size i = 0; i < _count; i++) {
    if (!read_float(result_[i])) {
      return false;
    }
  }

  return true;
}

bool Decoder::read_byte_array(Byte* result_, Size _count) {
  Uint64 count = 0;
  if (!read_uint(count)) {
    return false;
  }

  if (count != _count) {
    return error("array count mismatch");
  }

  return m_buffer.read_bytes(result_, _count);
}

bool Decoder::finalize() {
  return true;
}

bool Decoder::read_header() {
  auto header = reinterpret_cast<Byte*>(&m_header);
  if (m_stream->read(header, sizeof m_header) != sizeof m_header) {
    return error("read failed");
  }

  // Check fields of the header to see if they're correct.
  if (memcmp(m_header.magic, "REX", 4) != 0) {
    return error("malformed header");
  }

  // Sum of all sections and header should be the same size as the stream.
  Uint64 size = 0;
  size += sizeof m_header;
  size += m_header.data_size;
  size += m_header.string_size;

  if (size != *m_stream->size()) {
    return error("corrupted stream");
  }

  return true;
}

bool Decoder::read_strings() {
  // No need to read string table if empty.
  if (m_header.string_size == 0) {
    return true;
  }

  const auto cursor = m_stream->tell();

  // Seek to the strings offset.
  if (!m_stream->seek(m_header.data_size + sizeof m_header, Stream::Whence::SET)) {
    return error("seek failed");
  }

  LinearBuffer strings{allocator()};
  if (!strings.resize(m_header.string_size)) {
    return error("out of memory");
  }

  if (!m_stream->read(strings.data(), strings.size())) {
    return error("read failed");
  }

  // The last character in the string table is always a null-terminator.
  if (strings.last() != '\0') {
    return error("malformed string table");
  }

  auto table = StringTable::create_from_linear_buffer(Utility::move(strings));
  if (!table) {
    return error("out of memory");
  }

  m_strings = Utility::move(*table);

  // Restore the stream to where we were before we seeked and read in the strings
  if (!m_stream->seek(cursor, Stream::Whence::SET)) {
    return error("seek failed");
  }

  return true;
}

} // namespace Rx::serialize
