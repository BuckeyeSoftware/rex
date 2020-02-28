#ifndef RX_CORE_SERIALIZE_ENCODER_H
#define RX_CORE_SERIALIZE_ENCODER_H
#include "rx/core/serialize/header.h"
#include "rx/core/serialize/buffer.h"

#include "rx/core/traits/is_signed.h"
#include "rx/core/traits/is_unsigned.h"

#include "rx/core/string.h"
#include "rx/core/string_table.h"

namespace rx {
struct stream;
} // namespace rx

namespace rx::serialize {

struct encoder {
  encoder(stream* _stream);
  encoder(memory::allocator* _allocator, stream* _stream);
  ~encoder();

  [[nodiscard]] bool write_uint(rx_u64 _value);
  [[nodiscard]] bool write_sint(rx_s64 _value);
  [[nodiscard]] bool write_float(rx_f32 _value);
  [[nodiscard]] bool write_bool(bool _value);
  [[nodiscard]] bool write_byte(rx_byte _value);
  [[nodiscard]] bool write_string(const char* _string, rx_size _size);
  [[nodiscard]] bool write_string(const string& _string);

  [[nodiscard]] bool write_float_array(const rx_f32* _value, rx_size _count);
  [[nodiscard]] bool write_byte_array(const rx_byte* _data, rx_size _size);

  template<typename T>
  [[nodiscard]] bool write_uint_array(const T* _data, rx_size _count);

  template<typename T>
  [[nodiscard]] bool write_sint_array(const T* _data, rx_size _count);

  const string& message() const &;
  memory::allocator* allocator() const;

private:
  template<typename... Ts>
  bool error(const char* _format, Ts&&... _arguments);

  [[nodiscard]] bool write_header();
  [[nodiscard]] bool finalize();

  memory::allocator* m_allocator;
  stream* m_stream;

  header m_header;
  buffer m_buffer;
  string m_message;
  string_table m_strings;
};

inline encoder::encoder(stream* _stream)
  : encoder{&memory::g_system_allocator, _stream}
{
}

inline bool encoder::write_string(const string& _string) {
  return write_string(_string.data(), _string.size());
}

template<typename T>
inline bool encoder::write_uint_array(const T* _data, rx_size _count) {
  static_assert(traits::is_unsigned<T>, "T isn't unsigned integer type");

  if (!write_uint(_count)) {
    return false;
  }

  for (rx_size i = 0; i < _count; i++) {
    if (!write_uint(_data[i])) {
      return false;
    }
  }

  return true;
}

template<typename T>
inline bool encoder::write_sint_array(const T* _data, rx_size _count) {
  static_assert(traits::is_signed<T>, "T isn't signed integer type");

  if (!write_uint(_count)) {
    return false;
  }

  for (rx_size i = 0; i < _count; i++) {
    if (!write_uint(_data[i])) {
      return false;
    }
  }

  return true;
}

inline const string& encoder::message() const & {
  return m_message;
}

inline memory::allocator* encoder::allocator() const {
  return m_allocator;
}

template<typename... Ts>
inline bool encoder::error(const char* _format, Ts&&... _arguments) {
  m_message = string::format(_format, utility::forward<Ts>(_arguments)...);
  return false;
}

} // namespace rx::serialzie

#endif // RX_CORE_SERIALIZE_ENCODER_H
