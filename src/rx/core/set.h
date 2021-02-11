#ifndef RX_CORE_SET_H
#define RX_CORE_SET_H
#include "rx/core/traits/invoke_result.h"
#include "rx/core/traits/is_same.h"

#include "rx/core/concepts/trivially_destructible.h"

#include "rx/core/utility/swap.h"

#include "rx/core/hints/unreachable.h"

#include "rx/core/memory/system_allocator.h"
#include "rx/core/memory/aggregate.h"

namespace Rx {

// 32-bit: 28 bytes
// 64-bit: 56 bytes
template<typename K>
struct Set {
  RX_MARK_NO_COPY(Set);

  static inline constexpr const Size INITIAL_SIZE = 256;
  static inline constexpr const Size LOAD_FACTOR = 90;

  Set();
  Set(Memory::Allocator& _allocator);
  Set(Set&& set_);
  ~Set();

  static Optional<Set> copy(const Set& _set);

  Set& operator=(Set&& set_);

  K* insert(K&& _key);
  K* insert(const K& _key);

  K* find(const K& _key) const;

  bool erase(const K& _key);
  Size size() const;
  bool is_empty() const;

  void clear();

  template<typename F>
  bool each(F&& _function);
  template<typename F>
  bool each(F&& _function) const;

  constexpr Memory::Allocator& allocator() const;

private:
  void clear_and_deallocate();

  static Size hash_key(const K& _key);
  static bool is_deleted(Size _hash);

  Size desired_position(Size _hash) const;
  Size probe_distance(Size _hash, Size _slot_index) const;

  Size& element_hash(Size index);
  Size element_hash(Size index) const;

  [[nodiscard]] bool allocate(Size _capacity);
  [[nodiscard]] bool grow();

  // move and non-move construction functions
  K* construct(Size _index, Size _hash, K&& key_);

  K* inserter(Size _hash, K&& key_);
  K* inserter(Size _hash, const K& _key);

  bool lookup_index(const K& _key, Size& _index) const;

  Memory::Allocator* m_allocator;

  union {
    Byte* m_data;
    K* m_keys;
  };
  Size* m_hashes;

  Size m_size;
  Size m_capacity;
  Size m_resize_threshold;
  Size m_mask;
};

template<typename K>
Set<K>::Set()
  : Set{Memory::SystemAllocator::instance()}
{
}

template<typename K>
Set<K>::Set(Memory::Allocator& _allocator)
  : m_allocator{&_allocator}
  , m_keys{nullptr}
  , m_hashes{nullptr}
  , m_size{0}
  , m_capacity{0}
  , m_resize_threshold{0}
  , m_mask{0}
{
}

template<typename K>
Set<K>::Set(Set&& set_)
  : m_allocator{set_.m_allocator}
  , m_keys{Utility::exchange(set_.m_keys, nullptr)}
  , m_hashes{Utility::exchange(set_.m_hashes, nullptr)}
  , m_size{Utility::exchange(set_.m_size, 0)}
  , m_capacity{Utility::exchange(set_.m_capacity, 0)}
  , m_resize_threshold{Utility::exchange(set_.m_resize_threshold, 0)}
  , m_mask{Utility::exchange(set_.m_mask, 0)}
{
}

template<typename K>
Set<K>::~Set() {
  clear_and_deallocate();
}

template<typename K>
Optional<Set<K>> Set<K>::copy(const Set& _set) {
  Set<K> result;

  auto insert = [&result](const K& _key) {
    return result.insert(_key) != nullptr;
  };

  if (!_set.each(insert)) {
    return nullopt;
  }

  return result;
}

template<typename K>
void Set<K>::clear() {
  if (m_size == 0) {
    return;
  }

  for (Size i = 0; i < m_capacity; i++) {
    const Size hash = element_hash(i);
    if (hash == 0 || is_deleted(hash)) {
      continue;
    }

    if constexpr (!Concepts::TriviallyDestructible<K>) {
      Utility::destruct<K>(m_keys + i);
    }

    element_hash(i) = 0;
  }

  m_size = 0;
}

template<typename K>
void Set<K>::clear_and_deallocate() {
  clear();
  m_allocator->deallocate(m_data);
}

template<typename K>
Set<K>& Set<K>::operator=(Set<K>&& set_) {
  if (&set_ != this) {
    clear_and_deallocate();

    m_allocator = set_.m_allocator;
    m_keys = Utility::exchange(set_.m_keys, nullptr);
    m_hashes = Utility::exchange(set_.m_hashes, nullptr);
    m_size = Utility::exchange(set_.m_size, 0);
    m_capacity = Utility::exchange(set_.m_capacity, 0);
    m_resize_threshold = Utility::exchange(set_.m_resize_threshold, 0);
    m_mask = Utility::exchange(set_.m_mask, 0);
  }

  return *this;
}

template<typename K>
K* Set<K>::insert(K&& key_) {
  if (++m_size >= m_resize_threshold && !grow()) {
    return nullptr;
  }
  return inserter(hash_key(key_), Utility::forward<K>(key_));
}

template<typename K>
K* Set<K>::insert(const K& _key) {
  if (++m_size >= m_resize_threshold && !grow()) {
    return nullptr;
  }
  return inserter(hash_key(_key), _key);
}

template<typename K>
K* Set<K>::find(const K& _key) const {
  if (Size index; lookup_index(_key, index)) {
    return m_keys + index;
  }
  return nullptr;
}

template<typename K>
bool Set<K>::erase(const K& _key) {
  if (Size index; lookup_index(_key, index)) {
    if constexpr (!Concepts::TriviallyDestructible<K>) {
      Utility::destruct<K>(m_keys + index);
    }

    if constexpr (sizeof index == 8) {
      element_hash(index) |= 0x8000000000000000;
    } else {
      element_hash(index) |= 0x80000000;
    }

    m_size--;
    return true;
  }
  return false;
}

template<typename K>
Size Set<K>::size() const {
  return m_size;
}

template<typename K>
bool Set<K>::is_empty() const {
  return m_size == 0;
}

template<typename K>
Size Set<K>::hash_key(const K& _key) {
  auto hash_value = Hash::Hasher<K>{}(_key);

  // MSB is used to indicate deleted elements
  if constexpr(sizeof hash_value == 8) {
    hash_value &= 0x7FFFFFFFFFFFFFFF_z;
  } else {
    hash_value &= 0x7FFFFFFF_z;
  }

  // don't ever hash to zero since zero is used to indicate unused slots
  hash_value |= hash_value == 0;

  return hash_value;
}

template<typename K>
bool Set<K>::is_deleted(Size _hash) {
  // MSB indicates tombstones
  return (_hash >> ((sizeof _hash * 8) - 1)) != 0;
}

template<typename K>
Size Set<K>::desired_position(Size _hash) const {
  return _hash & m_mask;
}

template<typename K>
Size Set<K>::probe_distance(Size _hash, Size _slot_index) const {
  return (_slot_index + m_capacity - desired_position(_hash)) & m_mask;
}

template<typename K>
Size& Set<K>::element_hash(Size _index) {
  return m_hashes[_index];
}

template<typename K>
Size Set<K>::element_hash(Size _index) const {
  return m_hashes[_index];
}

template<typename K>
bool Set<K>::allocate(Size _capacity) {
  Memory::Aggregate aggregate;
  aggregate.add<K>(_capacity);
  aggregate.add<Size>(_capacity);
  if (!aggregate.finalize()) {
    return false;
  }

  auto data = m_allocator->allocate(aggregate.bytes());
  if (!data) {
    return false;
  }

  m_data = data;
  m_keys = reinterpret_cast<K*>(data + aggregate[0]);
  m_hashes = reinterpret_cast<Size*>(data + aggregate[1]);

  for (Size i = 0; i < _capacity; i++) {
    element_hash(i) = 0;
  }

  m_capacity = _capacity;
  m_resize_threshold = (m_capacity * LOAD_FACTOR) / 100;
  m_mask = m_capacity - 1;

  return true;
}

template<typename K>
bool Set<K>::grow() {
  const auto old_capacity = m_capacity;
  const auto new_capacity = m_capacity ? m_capacity * 2 : INITIAL_SIZE;

  auto data = m_data;
  auto keys = m_keys;
  auto hashes = m_hashes;

  if (!allocate(new_capacity)) {
    return false;
  }

  for (Size i = 0; i < old_capacity; i++) {
    const auto hash = hashes[i];
    if (hash == 0 || is_deleted(hash)) {
      continue;
    }

    RX_ASSERT(inserter(hash, Utility::move(keys[i])), "insertion failed");

    if constexpr (!Concepts::TriviallyDestructible<K>) {
      Utility::destruct<K>(keys + i);
    }
  }

  m_allocator->deallocate(data);

  return true;
}

template<typename K>
K* Set<K>::construct(Size _index, Size _hash, K&& key_) {
  Utility::construct<K>(m_keys + _index, Utility::forward<K>(key_));
  element_hash(_index) = _hash;
  return m_keys + _index;
}

template<typename K>
K* Set<K>::inserter(Size _hash, K&& key_) {
  Size position = desired_position(_hash);
  Size distance = 0;

  K* result = nullptr;
  for (;;) {
    if (element_hash(position) == 0) {
      K* insert = construct(position, _hash, Utility::forward<K>(key_));
      return result ? result : insert;
    }

    const Size existing_element_probe_distance{probe_distance(element_hash(position), position)};
    if (existing_element_probe_distance < distance) {
      if (is_deleted(element_hash(position))) {
        K* insert = construct(position, _hash, Utility::forward<K>(key_));
        return result ? result : insert;
      }

      if (!result) {
        result = m_keys + position;
      }

      Utility::swap(_hash, element_hash(position));
      Utility::swap(key_, m_keys[position]);

      distance = existing_element_probe_distance;
    }

    position = (position + 1) & m_mask;
    distance++;
  }

  return result;
}

template<typename K>
K* Set<K>::inserter(Size _hash, const K& _key) {
  K key{_key};
  return inserter(_hash, Utility::move(key));
}

template<typename K>
bool Set<K>::lookup_index(const K& _key, Size& _index) const {
  if (RX_HINT_UNLIKELY(m_size == 0)) {
    return false;
  }

  const Size hash{hash_key(_key)};
  Size position{desired_position(hash)};
  Size distance{0};
  for (;;) {
    const Size hash_element{element_hash(position)};
    if (hash_element == 0) {
      return false;
    } else if (distance > probe_distance(hash_element, position)) {
      return false;
    } else if (hash_element == hash && m_keys[position] == _key) {
      _index = position;
      return true;
    }
    position = (position + 1) & m_mask;
    distance++;
  }

  RX_HINT_UNREACHABLE();
}

template<typename K>
template<typename F>
bool Set<K>::each(F&& _function) {
  using ReturnType = Traits::InvokeResult<F, const K&>;
  for (Size i{0}; i < m_capacity; i++) {
    const auto hash{m_hashes[i]};
    if (hash != 0 && !is_deleted(hash)) {
      if constexpr (Traits::IS_SAME<ReturnType, bool>) {
        if (!_function(m_keys[i])) {
          return false;
        }
      } else {
        _function(m_keys[i]);
      }
    }
  }
  return true;
}

template<typename K>
template<typename F>
bool Set<K>::each(F&& _function) const {
  using ReturnType = Traits::InvokeResult<F, const K&>;
  for (Size i{0}; i < m_capacity; i++) {
    const auto hash{m_hashes[i]};
    if (hash != 0 && !is_deleted(hash)) {
      if constexpr (Traits::IS_SAME<ReturnType, bool>) {
        if (!_function(m_keys[i])) {
          return false;
        }
      } else {
        _function(m_keys[i]);
      }
    }
  }
  return true;
}

template<typename K>
RX_HINT_FORCE_INLINE constexpr Memory::Allocator& Set<K>::allocator() const {
  return *m_allocator;
}

} // namespace Rx

#endif // RX_CORE_SET
