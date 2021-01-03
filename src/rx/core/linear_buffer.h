#ifndef RX_CORE_BUFFER_H
#define RX_CORE_BUFFER_H
#include "rx/core/memory/system_allocator.h"
#include "rx/core/memory/uninitialized_storage.h"

#include "rx/core/optional.h"
#include "rx/core/assert.h"

namespace Rx {

// 32-bit: 16 + INSITU_SIZE bytes
// 64-bit: 32 + INSITU_SIZE bytes
struct LinearBuffer {
  RX_MARK_NO_COPY(LinearBuffer);

  static inline constexpr const Size INSITU_SIZE = 4096;
  static inline constexpr const Size INSITU_ALIGNMENT = Memory::Allocator::ALIGNMENT;

  constexpr LinearBuffer();
  constexpr LinearBuffer(Memory::Allocator& _allocator);

  ~LinearBuffer();
  LinearBuffer(LinearBuffer&& buffer_);
  LinearBuffer& operator=(LinearBuffer&& buffer_);

  void clear();

  void erase(Size _from, Size _to);

  Byte* data();
  const Byte* data() const;

  Byte& operator[](Size _index);
  const Byte& operator[](Size _index) const;

  Byte& last();
  const Byte& last() const;

  Size size() const;
  Size capacity() const;

  constexpr Memory::Allocator& allocator() const;

  bool in_situ() const;
  bool is_empty() const;
  bool in_range(Size _index) const;

  [[nodiscard]] bool push_back(Byte _value);
  [[nodiscard]] bool reserve(Size _size);
  /*[[nodiscard]]*/ bool resize(Size _size);

  Optional<Memory::View> disown();

private:
  void release();

  Memory::Allocator* m_allocator;
  Byte* m_data;
  Size m_size;
  Size m_capacity;
  Memory::UninitializedStorage<INSITU_SIZE, INSITU_ALIGNMENT> m_insitu;
};

inline constexpr LinearBuffer::LinearBuffer()
  : LinearBuffer{Memory::SystemAllocator::instance()}
{
}

inline constexpr LinearBuffer::LinearBuffer(Memory::Allocator& _allocator)
  : m_allocator{&_allocator}
  , m_data{nullptr}
  , m_size{0}
  , m_capacity{INSITU_SIZE}
{
  m_data = m_insitu.data();
}

inline LinearBuffer::~LinearBuffer() {
  release();
}

inline void LinearBuffer::clear() {
  m_size = 0;
}

inline Byte* LinearBuffer::data() {
  return m_data;
}

inline const Byte* LinearBuffer::data() const {
  return m_data;
}

inline Byte& LinearBuffer::operator[](Size _index) {
  RX_ASSERT(_index < m_size, "index out of bounds");
  return m_data[_index];
}

inline const Byte& LinearBuffer::operator[](Size _index) const {
  RX_ASSERT(_index < m_size, "index out of bounds");
  return m_data[_index];
}

inline Byte& LinearBuffer::last() {
  RX_ASSERT(m_size, "empty");
  return m_data[m_size - 1];
}

inline const Byte& LinearBuffer::last() const {
  RX_ASSERT(m_size, "empty");
  return m_data[m_size - 1];
}

inline Size LinearBuffer::size() const {
  return m_size;
}

inline Size LinearBuffer::capacity() const {
  return m_capacity;
}

inline constexpr Memory::Allocator& LinearBuffer::allocator() const {
  return *m_allocator;
}

inline bool LinearBuffer::in_situ() const {
  return m_data == m_insitu.data();
}

inline bool LinearBuffer::is_empty() const {
  return m_size == 0;
}

inline bool LinearBuffer::in_range(Size _index) const {
  return _index < m_size;
}

} // namespace Rx

#endif // RX_CORE_LINEAR_BUFFER_H
