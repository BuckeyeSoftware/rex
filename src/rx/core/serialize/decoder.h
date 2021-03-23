#ifndef RX_CORE_SERIALIZE_DECODER_H
#define RX_CORE_SERIALIZE_DECODER_H
#include "rx/core/serialize/buffer.h"
#include "rx/core/serialize/header.h"

#include "rx/core/concepts/integral.h"

#include "rx/core/string.h"
#include "rx/core/string_table.h"

namespace Rx {

struct Stream;

} // namespace Rx

namespace Rx::serialize {

struct RX_API Decoder {
  Decoder(Stream& _stream);
  Decoder(Memory::Allocator& _allocator, Stream& _stream);
  ~Decoder();

  [[nodiscard]] bool read_uint(Uint64& result_);
  [[nodiscard]] bool read_sint(Sint64& result_);
  [[nodiscard]] bool read_float(Float32& result_);
  [[nodiscard]] bool read_bool(bool& result_);
  [[nodiscard]] bool read_byte(Byte& result_);
  [[nodiscard]] bool read_string(String& result_);

  [[nodiscard]] bool read_float_array(Float32* result_, Size _count);
  [[nodiscard]] bool read_byte_array(Byte* result_, Size _count);

  template<Concepts::UnsignedIntegral T>
  [[nodiscard]] bool read_uint_array(T* result_, Size _count);

  template<Concepts::SignedIntegral T>
  [[nodiscard]] bool read_sint_array(T* result_, Size _count);

  const String& message() const &;
  constexpr Memory::Allocator& allocator() const;

private:
  template<typename... Ts>
  [[nodiscard]] RX_HINT_FORMAT(2, 0)
  bool error(const char* _format, Ts&&... _arguments);

  [[nodiscard]] bool read_header();
  [[nodiscard]] bool read_strings();
  [[nodiscard]] bool finalize();

  Memory::Allocator& m_allocator;
  Stream* m_stream;

  Header m_header;
  Buffer m_buffer;
  String m_message;
  StringTable m_strings;
};

inline Decoder::Decoder(Stream& _stream)
  : Decoder{Memory::SystemAllocator::instance(), _stream}
{
}

template<Concepts::UnsignedIntegral T>
bool Decoder::read_uint_array(T* result_, Size _count) {
  Uint64 count = 0;
  if (!read_uint(count)) {
    return false;
  }

  if (count != _count) {
    return false;
  }

  for (Size i = 0; i < _count; i++) {
    if (!read_uint(result_[i])) {
      return error("array count mismatch");
    }
  }

  return true;
}

template<Concepts::SignedIntegral T>
bool Decoder::read_sint_array(T* result_, Size _count) {
  Uint64 count = 0;
  if (!read_uint(count)) {
    return false;
  }

  if (count != _count) {
    return error("array count mismatch");
  }

  for (Size i = 0; i < _count; i++) {
    if (!read_uint(result_[i])) {
      return false;
    }
  }

  return true;
}

inline const String& Decoder::message() const & {
  return m_message;
}

RX_HINT_FORCE_INLINE constexpr Memory::Allocator& Decoder::allocator() const {
  return m_allocator;
}

template<typename... Ts>
bool Decoder::error(const char* _format, Ts&&... _arguments) {
  m_message = String::format(_format, Utility::forward<Ts>(_arguments)...);
  return false;
}

} // namespace Rx::serialize

#endif // RX_CORE_SERIALIZE_DECODER_H
