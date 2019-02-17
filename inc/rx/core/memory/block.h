#ifndef RX_CORE_MEMORY_BLOCK_H
#define RX_CORE_MEMORY_BLOCK_H

#include <rx/core/assert.h> // RX_ASSERT
#include <rx/core/types.h> // rx_{size, byte}
#include <rx/core/concepts/no_copy.h> // no_copy

namespace rx::memory {

struct block : concepts::no_copy {
  constexpr block();
  constexpr block(rx_size size, rx_byte* data);
  constexpr block(block&& other);
  constexpr void operator=(block&& other);
  constexpr operator bool() const;
  constexpr rx_size size() const;
  constexpr rx_byte* data() const;
  constexpr rx_byte* end() const;
  template<typename T>
  constexpr T cast() const;
private:
  rx_size m_size;
  rx_byte* m_data;
};

inline constexpr block::block()
  : m_size{0}
  , m_data{nullptr}
{
}

inline constexpr block::block(rx_size size, rx_byte* data)
  : m_size{size}
  , m_data{data}
{
}

inline constexpr block::block(block&& other)
  : m_size{other.m_size}
  , m_data{other.m_data}
{
  other.m_size = 0;
  other.m_data = nullptr;
}

inline constexpr void block::operator=(block&& other) {
  m_size = other.m_size;
  m_data = other.m_data;

  other.m_size = 0;
  other.m_data = nullptr;
}

inline constexpr block::operator bool() const {
  return !!m_data;
}

inline constexpr rx_size block::size() const {
  RX_ASSERT(m_data, "empty block");
  return m_size;
}

inline constexpr rx_byte* block::data() const {
  RX_ASSERT(m_data, "empty block");
  return m_data;
}

inline constexpr rx_byte* block::end() const {
  RX_ASSERT(m_data, "empty block");
  return m_data + m_size;
}

template<typename T>
inline constexpr T block::cast() const {
  RX_ASSERT(m_data, "empty block");
  return reinterpret_cast<T>(m_data);
}

} // namespace rx::memory

#endif // RX_CORE_MEMORY_BLOCK_H
