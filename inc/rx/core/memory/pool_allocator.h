#ifndef RX_CORE_MEMORY_POOL_ALLOCATOR_H
#define RX_CORE_MEMORY_POOL_ALLOCATOR_H

#include <rx/core/bitset.h> // bitset

#include <rx/core/concepts/no_copy.h> // no_copy
#include <rx/core/concepts/no_move.h> // no_move

#include <rx/core/utility/construct.h> // construct
#include <rx/core/utility/destruct.h> // destruct
#include <rx/core/utility/forward.h> // forward

namespace rx::memory {

struct allocator;

struct pool_allocator
  : concepts::no_copy
  , concepts::no_move
{
  pool_allocator() = default;
  pool_allocator(allocator* _allocator, rx_size _size, rx_size _count);
  ~pool_allocator();

  rx_byte* allocate();
  void deallocate(rx_byte* _data);
  bool owns(const rx_byte* _data) const;
  rx_size index_of(const rx_byte* _data) const;
  rx_byte* data_of(rx_size index) const;
  rx_size capacity() const;
  rx_size size() const;

  template<typename T, typename... Ts>
  T* allocate_and_construct(Ts&&... _arguments);

  template<typename T>
  void destruct_and_deallocate(T* _data);

private:
  allocator* m_allocator;
  rx_size m_object_size;
  rx_size m_object_count;

  rx_byte* m_data;
  bitset m_bits;
};

inline bool pool_allocator::owns(const rx_byte* _data) const {
  return _data >= m_data && _data + m_object_size <= m_data + m_object_size * m_object_count;
}

inline rx_size pool_allocator::index_of(const rx_byte* _data) const {
  RX_ASSERT(owns(_data), "pool does not own memory");
  return (_data - m_data) / m_object_size;
}

inline rx_byte* pool_allocator::data_of(rx_size _index) const {
  return m_data + m_object_size * _index;
}

inline rx_size pool_allocator::capacity() const {
  return m_object_count;
}

inline rx_size pool_allocator::size() const {
  return m_bits.count_set_bits();
}

template<typename T, typename... Ts>
inline T* pool_allocator::allocate_and_construct(Ts&&... _arguments) {
  RX_ASSERT(sizeof(T) <= m_object_size, "size too large");
  auto data{allocate()};
  RX_ASSERT(data, "out of memory");
  utility::construct<T>(data, utility::forward<Ts>(_arguments)...);
  return reinterpret_cast<T*>(data);
}

template<typename T>
inline void pool_allocator::destruct_and_deallocate(T* _data) {
  auto raw_data{reinterpret_cast<rx_byte*>(_data)};
  RX_ASSERT(sizeof(T) <= m_object_size, "size too large");
  RX_ASSERT(owns(raw_data), "does not own memory");
  utility::destruct<T>(raw_data);
  deallocate(raw_data);
}

} // namespace rx::memory

#endif // RX_CORE_MEMORY_POOL_ALLOCATOR_H

