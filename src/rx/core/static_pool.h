#ifndef RX_CORE_STATIC_POOL_H
#define RX_CORE_STATIC_POOL_H
#include "rx/core/bitset.h"
#include "rx/core/assert.h"

#include "rx/core/hints/unlikely.h"

namespace Rx {

struct RX_API StaticPool {
  RX_MARK_NO_COPY(StaticPool);

  static Optional<StaticPool> create(Memory::Allocator& _allocator,
    Size _object_size, Size _object_count);

  StaticPool(StaticPool&& pool_);
  ~StaticPool();

  StaticPool& operator=(StaticPool&& pool_);
  Byte* operator[](Size _index) const;

  Optional<Size> allocate();
  void deallocate(Size _index);

  template<typename T, typename... Ts>
  T* create(Ts&&... _arguments);

  template<typename T>
  void destroy(T* _data);

  constexpr Memory::Allocator& allocator() const;

  Size object_size() const;
  Size capacity() const;
  Size size() const;
  bool is_empty() const;
  bool can_allocate() const;

  Byte* data_of(Size _index) const;
  Size index_of_untyped(const Byte* _data) const;

  template<typename T>
  Size index_of(const T* _data) const;

  bool owns(const Byte* _data) const;

private:
  StaticPool(Memory::Allocator& _allocator, Size _object_size, Size _object_count, Byte* _data, Bitset&& bitset_)
    : m_allocator{&_allocator}
    , m_object_size{_object_size}
    , m_object_count{_object_count}
    , m_data{_data}
    , m_bitset{Utility::move(bitset_)}
  {
  }

  Memory::Allocator* m_allocator;
  Size m_object_size;
  Size m_object_count;
  Byte* m_data;
  Bitset m_bitset;
};

inline StaticPool::~StaticPool() {
  RX_ASSERT(m_bitset.count_set_bits() == 0, "leaked objects");
  m_allocator->deallocate(m_data);
}

inline Byte* StaticPool::operator[](Size _index) const {
  return data_of(_index);
}

template<typename T, typename... Ts>
T* StaticPool::create(Ts&&... _arguments) {
  RX_ASSERT(sizeof(T) <= m_object_size, "object too large (%zu > %zu)",
    sizeof(T), m_object_size);

  const auto index = allocate();
  if (RX_HINT_UNLIKELY(!index)) {
    return nullptr;
  }

  return Utility::construct<T>(data_of(*index), Utility::forward<Ts>(_arguments)...);
}

template<typename T>
void StaticPool::destroy(T* _data) {
  RX_ASSERT(sizeof(T) <= m_object_size, "object too large (%zu > %zu)",
    sizeof(T), m_object_size);

  Utility::destruct<T>(_data);
  deallocate(index_of(_data));
}

RX_HINT_FORCE_INLINE constexpr Memory::Allocator& StaticPool::allocator() const {
  return *m_allocator;
}

RX_HINT_FORCE_INLINE Size StaticPool::object_size() const {
  return m_object_size;
}

RX_HINT_FORCE_INLINE Size StaticPool::capacity() const {
  return m_object_count;
}

RX_HINT_FORCE_INLINE Size StaticPool::size() const {
  return m_bitset.count_set_bits();
}

RX_HINT_FORCE_INLINE bool StaticPool::is_empty() const {
  return size() == 0;
}

inline bool StaticPool::can_allocate() const {
  return m_bitset.count_unset_bits() != 0;
}

inline Byte* StaticPool::data_of(Size _index) const {
  RX_ASSERT(_index < m_object_count, "out of bounds");
  RX_ASSERT(m_bitset.test(_index), "unallocated (%zu)", _index);
  return m_data + m_object_size * _index;
}

inline Size StaticPool::index_of_untyped(const Byte* _data) const {
  RX_ASSERT(owns(_data), "invalid pointer");
  return (_data - m_data) / m_object_size;
}

template<typename T>
Size StaticPool::index_of(const T* _data) const {
  return index_of_untyped(reinterpret_cast<const Byte*>(_data));
}

inline bool StaticPool::owns(const Byte* _data) const {
  return _data >= m_data && _data <= m_data + m_object_size * (m_object_count - 1);
}

} // namespace Rx

#endif // RX_CORE_POOL_H
