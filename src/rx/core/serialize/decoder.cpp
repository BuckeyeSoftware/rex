#include "rx/core/serialize/decoder.h"
#include "rx/core/utility/byte_swap.h"
#include "rx/core/math/shape.h"

namespace Rx::Serialize {

Optional<Decoder> Decoder::create(Memory::Allocator& _allocator,
  Stream::Context& _stream)
{
  if (!(_stream.flags() & Stream::READ)) {
    // Stream does not support reading from.
    return nullopt;
  }

  auto buffer = Stream::BufferedStream::create(_allocator);
  if (!buffer || !buffer->attach(_stream)) {
    // Couldn't create buffered stream or attach this one.
    return nullopt;
  }

  return Decoder { Utility::move(*buffer) };
}

template<typename T>
static bool read(auto& stream_, T& value_) {
  const auto data = reinterpret_cast<Byte*>(&value_);
  return stream_.read(data, sizeof value_) == sizeof value_;
}

static constexpr bool is_big_endian() { return false; }
static constexpr bool is_little_endian() { return true; }

Optional<Uint8> Decoder::get_u8() {
  if (Uint8 value; read(m_stream, value)) {
    return value;
  }
  return nullopt;
}

Optional<Sint8> Decoder::get_s8() {
  if (Sint8 value; read(m_stream, value)) {
    return value;
  }
  return nullopt;
}

Optional<Uint16> Decoder::get_u16le() {
  if (Uint16 value; read(m_stream, value)) {
    return is_big_endian() ? Utility::swap_u16(value) : value;
  }
  return nullopt;
}

Optional<Sint16> Decoder::get_s16le() {
  if (Sint16 value; read(m_stream, value)) {
    return is_big_endian() ? Utility::swap_s16(value) : value;
  }
  return nullopt;
}

Optional<Uint16> Decoder::get_u16be() {
  if (Uint16 value; read(m_stream, value)) {
    return is_little_endian() ? Utility::swap_u16(value) : value;
  }
  return nullopt;
}

Optional<Sint16> Decoder::get_s16be() {
  if (Sint16 value; read(m_stream, value)) {
    return is_little_endian() ? Utility::swap_s16(value) : value;
  }
  return nullopt;
}

Optional<Uint32> Decoder::get_u32le() {
  if (Uint32 value; read(m_stream, value)) {
    return is_big_endian() ? Utility::swap_u32(value) : value;
  }
  return nullopt;
}

Optional<Sint32> Decoder::get_s32le() {
  if (Sint32 value; read(m_stream, value)) {
    return is_big_endian() ? Utility::swap_s32(value) : value;
  }
  return nullopt;
}

Optional<Uint32> Decoder::get_u32be() {
  if (Uint32 value; read(m_stream, value)) {
    return is_little_endian() ? Utility::swap_u32(value) : value;
  }
  return nullopt;
}

Optional<Sint32> Decoder::get_s32be() {
  if (Sint32 value; read(m_stream, value)) {
    return is_little_endian() ? Utility::swap_s32(value) : value;
  }
  return nullopt;
}

Optional<Uint64> Decoder::get_u64le() {
  if (Uint64 value; read(m_stream, value)) {
    return is_big_endian() ? Utility::swap_u64(value) : value;
  }
  return nullopt;
}

Optional<Sint64> Decoder::get_s64le() {
  if (Sint64 value; read(m_stream, value)) {
    return is_big_endian() ? Utility::swap_s64(value) : value;
  }
  return nullopt;
}

Optional<Uint64> Decoder::get_u64be() {
  if (Uint64 value; read(m_stream, value)) {
    return is_little_endian() ? Utility::swap_u64(value) : value;
  }
  return nullopt;
}

Optional<Sint64> Decoder::get_s64be() {
  if (Sint64 value; read(m_stream, value)) {
    return is_little_endian() ? Utility::swap_s64(value) : value;
  }
  return nullopt;
}

Optional<Float32> Decoder::get_f32le() {
  if (const auto u32 = get_u32le()) {
    return Math::Shape{*u32}.as_f32;
  }
  return nullopt;
}

Optional<Float32> Decoder::get_f32be() {
  if (const auto u32 = get_u32be()) {
    return Math::Shape{*u32}.as_f32;
  }
  return nullopt;
}

Optional<Float64> Decoder::get_f64le() {
  if (const auto u64 = get_u64le()) {
    return Math::Shape{*u64}.as_f64;
  }
  return nullopt;
}

Optional<Float64> Decoder::get_f64be() {
  if (const auto u64 = get_u64be()) {
    return Math::Shape{*u64}.as_f64;
  }
  return nullopt;
}

} // namespace Rx::Serialize