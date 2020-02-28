#ifndef RX_CORE_SERIALIZE_DECODER_H
#define RX_CORE_SERIALIZE_DECODER_H
#include "rx/core/serialize/buffer.h"
#include "rx/core/serialize/header.h"

#include "rx/core/traits/is_signed.h"
#include "rx/core/traits/is_unsigned.h"

#include "rx/core/string.h"
#include "rx/core/string_table.h"

namespace rx {
struct stream;
} // namespace rx

namespace rx::serialize {

struct decoder {
  decoder(stream* _stream);
  decoder(memory::allocator* _allocator, stream* _stream);
  ~decoder();

  [[nodiscard]] bool read_uint(rx_u64& result_);
  [[nodiscard]] bool read_sint(rx_s64& result_);
  [[nodiscard]] bool read_float(rx_f32& result_);
  [[nodiscard]] bool read_bool(bool& result_);
  [[nodiscard]] bool read_byte(rx_byte& result_);
  [[nodiscard]] bool read_string(string& result_);

  [[nodiscard]] bool read_float_array(rx_f32* result_, rx_size _count);
  [[nodiscard]] bool read_byte_array(rx_byte* result_, rx_size _count);

  template<typename T>
  [[nodiscard]] bool read_uint_array(T* result_, rx_size _count);

  template<typename T>
  [[nodiscard]] bool read_sint_array(T* result_, rx_size _count);

  const string& message() const &;
  memory::allocator* allocator() const;

private:
  template<typename... Ts>
  bool error(const char* _format, Ts&&... _arguments);

  [[nodiscard]] bool read_header();
  [[nodiscard]] bool read_strings();
  [[nodiscard]] bool finalize();

  memory::allocator* m_allocator;
  stream* m_stream;

  header m_header;
  buffer m_buffer;
  string m_message;
  memory::uninitialized_storage<string_table> m_strings;
};

inline decoder::decoder(stream* _stream)
  : decoder{&memory::g_system_allocator, _stream}
{
}

template<typename T>
inline bool decoder::read_uint_array(T* result_, rx_size _count) {
  static_assert(traits::is_unsigned<T>, "T must be unsigned integer");

  rx_u64 count = 0;
  if (!read_uint(count)) {
    return false;
  }

  if (count != _count) {
    return false;
  }

  for (rx_size i = 0; i < _count; i++) {
    if (!read_uint(result_[i])) {
      return error("array count mismatch");
    }
  }

  return true;
}

template<typename T>
inline bool decoder::read_sint_array(T* result_, rx_size _count) {
  static_assert(traits::is_signed<T>, "T must be signed integer");

  rx_u64 count = 0;
  if (!read_uint(count)) {
    return false;
  }

  if (count != _count) {
    return error("array count mismatch");
  }

  for (rx_size i = 0; i < _count; i++) {
    if (!read_uint(result_[i])) {
      return false;
    }
  }

  return true;
}

inline const string& decoder::message() const & {
  return m_message;
}

inline memory::allocator* decoder::allocator() const {
  return m_allocator;
}

template<typename... Ts>
inline bool decoder::error(const char* _format, Ts&&... _arguments) {
  m_message = string::format(_format, utility::forward<Ts>(_arguments)...);
  return false;
}

} // namespace rx::serialize

#endif // RX_CORE_SERIALIZE_DECODER_H
