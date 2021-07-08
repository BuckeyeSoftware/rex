#include "rx/core/serialize/encoder.h"

#include "rx/core/math/shape.h"
#include "rx/core/utility/byte_swap.h"

namespace Rx::Serialize {

using namespace Rx::Utility;

Optional<Encoder> Encoder::create(Memory::Allocator& _allocator,
  Stream::Context& _stream)
{
  if (!(_stream.flags() & Stream::WRITE)) {
    // Stream does not support writing to.
    return nullopt;
  }

  auto buffer = Stream::BufferedStream::create(_allocator);
  if (!buffer || !buffer->attach(_stream)) {
    // Couldn't create buffered stream or attach this one.
    return nullopt;
  }

  return Encoder { Utility::move(*buffer) };
}

template<typename T>
static bool write(auto&& stream_, const T& _value) {
  const auto data = reinterpret_cast<const Byte*>(&_value);
  return stream_.write(data, sizeof _value) == sizeof _value;
}

static constexpr bool is_big_endian() { return false; }
static constexpr bool is_little_endian() { return true; }

bool Encoder::put_u8(Uint8 _value) {
  return write(m_stream, _value);
}

bool Encoder::put_s8(Sint8 _value) {
  return write(m_stream, _value);
}

bool Encoder::put_u16le(Uint16 _value) {
  return write(m_stream, is_big_endian() ? swap_u16(_value) : _value);
}

bool Encoder::put_s16le(Sint16 _value) {
  return write(m_stream, is_big_endian() ? swap_s16(_value) : _value);
}

bool Encoder::put_u16be(Uint16 _value) {
  return write(m_stream, is_little_endian() ? swap_u16(_value) : _value);
}

bool Encoder::put_s16be(Sint16 _value) {
  return write(m_stream, is_little_endian() ? swap_s16(_value) : _value);
}

bool Encoder::put_u32le(Uint32 _value) {
  return write(m_stream, is_big_endian() ? swap_u32(_value) : _value);
}

bool Encoder::put_s32le(Sint32 _value) {
  return write(m_stream, is_big_endian() ? swap_s32(_value) : _value);
}

bool Encoder::put_u32be(Uint32 _value) {
  return write(m_stream, is_little_endian() ? swap_u32(_value) : _value);
}

bool Encoder::put_s32be(Sint32 _value) {
  return write(m_stream, is_little_endian() ? swap_s32(_value) : _value);
}

bool Encoder::put_u64le(Uint64 _value) {
  return write(m_stream, is_big_endian() ? swap_u64(_value) : _value);
}

bool Encoder::put_s64le(Uint64 _value) {
  return write(m_stream, is_big_endian() ? swap_s64(_value) : _value);
}

bool Encoder::put_u64be(Uint64 _value) {
  return write(m_stream, is_little_endian() ? swap_u64(_value) : _value);
}

bool Encoder::put_s64be(Uint64 _value) {
  return write(m_stream, is_little_endian() ? swap_s64(_value) : _value);
}

bool Encoder::put_f32le(Float32 _value) {
  return put_u32le(Math::Shape{_value}.as_u32);
}

bool Encoder::put_f32be(Float32 _value) {
  return put_u32be(Math::Shape{_value}.as_u32);
}

bool Encoder::put_f64le(Float64 _value) {
  return put_u64le(Math::Shape{_value}.as_u64);
}

bool Encoder::put_f64be(Float64 _value) {
  return put_u64be(Math::Shape{_value}.as_u64);
}

} // namespace Rx::Serialize