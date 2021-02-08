#ifndef RX_CORE_MAP_H
#define RX_CORE_MAP_H
#include "rx/core/traits/invoke_result.h"
#include "rx/core/traits/is_same.h"

#include "rx/core/concepts/trivially_destructible.h"

#include "rx/core/utility/swap.h"
#include "rx/core/utility/pair.h"
#include "rx/core/utility/copy.h"

#include "rx/core/hints/unreachable.h"
#include "rx/core/hints/unlikely.h"

#include "rx/core/memory/system_allocator.h"
#include "rx/core/memory/aggregate.h"

#include "rx/core/hash/hasher.h"

namespace Rx {

// 32-bit: 28 bytes
// 64-bit: 56 bytes
template<typename K, typename V>
struct Map {
  RX_MARK_NO_COPY(Map);

  static inline constexpr const Size INITIAL_SIZE = 256;
  static inline constexpr const Size LOAD_FACTOR = 90;

  Map();
  Map(Memory::Allocator& _allocator);
  Map(Map&& map_);
  ~Map();

  static Optional<Map> copy(const Map& _map);

  Map& operator=(Map&& map_);

  V* insert(const K& _key, V&& value_);
  V* insert(const K& _key, const V& _value);

  V* find(const K& _key);
  const V* find(const K& _key) const;

  bool erase(const K& _key);
  Size size() const;
  bool is_empty() const;

  void clear();

  template<typename F>
  bool each_key(F&& _function);
  template<typename F>
  bool each_key(F&& _function) const;

  template<typename F>
  bool each_value(F&& _function);
  template<typename F>
  bool each_value(F&& _function) const;

  template<typename F>
  bool each_pair(F&& _function);
  template<typename F>
  bool each_pair(F&& _function) const;

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
  V* construct(Size _index, Size _hash, K&& key_, V&& value_);

  V* inserter(Size _hash, K&& key_, V&& value_);
  V* inserter(Size _hash, const K& _key, const V& _value);
  V* inserter(Size _hash, const K& _key, V&& value_);

  bool lookup_index(const K& _key, Size& _index) const;

  Memory::Allocator* m_allocator;

  union {
    Byte* m_data;
    K* m_keys;
  };
  V* m_values;
  Size* m_hashes;

  Size m_size;
  Size m_capacity;
  Size m_resize_threshold;
  Size m_mask;
};

template<typename K, typename V>
Map<K, V>::Map()
  : Map{Memory::SystemAllocator::instance()}
{
}

template<typename K, typename V>
Map<K, V>::Map(Memory::Allocator& _allocator)
  : m_allocator{&_allocator}
  , m_keys{nullptr}
  , m_values{nullptr}
  , m_hashes{nullptr}
  , m_size{0}
  , m_capacity{0}
  , m_resize_threshold{0}
  , m_mask{0}
{
}

template<typename K, typename V>
Map<K, V>::Map(Map&& map_)
  : m_allocator{&map_.allocator()}
  , m_keys{Utility::exchange(map_.m_keys, nullptr)}
  , m_values{Utility::exchange(map_.m_values, nullptr)}
  , m_hashes{Utility::exchange(map_.m_hashes, nullptr)}
  , m_size{Utility::exchange(map_.m_size, 0)}
  , m_capacity{Utility::exchange(map_.m_capacity, 0)}
  , m_resize_threshold{Utility::exchange(map_.m_resize_threshold, 0)}
  , m_mask{Utility::exchange(map_.m_mask, 0)}
{
}

/*
template<typename K, typename V>
Map<K, V>::Map(const Map& _map)
  : Map{_map.allocator()}
{
  _map.each_pair([this](const K& _key, const V& _value) { insert(_key, _value); });
}*/

template<typename K, typename V>
Map<K, V>::~Map() {
  clear_and_deallocate();
}

template<typename K, typename V>
Optional<Map<K, V>> Map<K, V>::copy(const Map& _map) {
  Map<K, V> result;

  auto insert = [&result](const K& _key, const V& _value) {
    return result.insert(_key, _value) != nullptr;
  };

  if (!_map.each_pair(insert)) {
    return nullopt;
  }

  return result;
}

template<typename K, typename V>
void Map<K, V>::clear() {
  if (m_size == 0) {
    return;
  }

  for (Size i = 0; i < m_capacity; i++) {
    const auto hash = element_hash(i);
    if (hash == 0 || is_deleted(hash)) {
      continue;
    }

    if constexpr (!Concepts::TriviallyDestructible<K>) {
      Utility::destruct<K>(m_keys + i);
    }

    if constexpr (!Concepts::TriviallyDestructible<V>) {
      Utility::destruct<V>(m_values + i);
    }

    element_hash(i) = 0;
  }

  m_size = 0;
}

template<typename K, typename V>
void Map<K, V>::clear_and_deallocate() {
  clear();
  m_allocator->deallocate(m_data);
}

template<typename K, typename V>
Map<K, V>& Map<K, V>::operator=(Map<K, V>&& map_) {
  RX_ASSERT(&map_ != this, "self assignment");

  clear_and_deallocate();

  m_allocator = map_.m_allocator;
  m_keys = Utility::exchange(map_.m_keys, nullptr);
  m_values = Utility::exchange(map_.m_values, nullptr);
  m_hashes = Utility::exchange(map_.m_hashes, nullptr);
  m_size = Utility::exchange(map_.m_size, 0);
  m_capacity = Utility::exchange(map_.m_capacity, 0);
  m_resize_threshold = Utility::exchange(map_.m_resize_threshold, 0);
  m_mask = Utility::exchange(map_.m_mask, 0);

  return *this;
}

template<typename K, typename V>
V* Map<K, V>::insert(const K& _key, V&& value_) {
  if (++m_size >= m_resize_threshold && !grow()) {
    return nullptr;
  }
  return inserter(hash_key(_key), _key, Utility::forward<V>(value_));
}

template<typename K, typename V>
V* Map<K, V>::insert(const K& _key, const V& _value) {
  if (++m_size >= m_resize_threshold && !grow()) {
    return nullptr;
  }
  return inserter(hash_key(_key), _key, _value);
}

template<typename K, typename V>
V* Map<K, V>::find(const K& _key) {
  if (Size index; lookup_index(_key, index)) {
    return m_values + index;
  }
  return nullptr;
}

template<typename K, typename V>
const V* Map<K, V>::find(const K& _key) const {
  if (Size index; lookup_index(_key, index)) {
    return m_values + index;
  }
  return nullptr;
}

template<typename K, typename V>
bool Map<K, V>::erase(const K& _key) {
  if (Size index; lookup_index(_key, index)) {
    if constexpr (!Concepts::TriviallyDestructible<K>) {
      Utility::destruct<K>(m_keys + index);
    }
    if constexpr (!Concepts::TriviallyDestructible<V>) {
      Utility::destruct<V>(m_values + index);
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

template<typename K, typename V>
Size Map<K, V>::size() const {
  return m_size;
}

template<typename K, typename V>
bool Map<K, V>::is_empty() const {
  return m_size == 0;
}

template<typename K, typename V>
Size Map<K, V>::hash_key(const K& _key) {
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

template<typename K, typename V>
bool Map<K, V>::is_deleted(Size _hash) {
  // MSB indicates tombstones
  return (_hash >> ((sizeof _hash * 8) - 1)) != 0;
}

template<typename K, typename V>
Size Map<K, V>::desired_position(Size _hash) const {
  return _hash & m_mask;
}

template<typename K, typename V>
Size Map<K, V>::probe_distance(Size _hash, Size _slot_index) const {
  return (_slot_index + m_capacity - desired_position(_hash)) & m_mask;
}

template<typename K, typename V>
Size& Map<K, V>::element_hash(Size _index) {
  return m_hashes[_index];
}

template<typename K, typename V>
Size Map<K, V>::element_hash(Size _index) const {
  return m_hashes[_index];
}

template<typename K, typename V>
bool Map<K, V>::allocate(Size _capacity) {
  Memory::Aggregate aggregate;
  aggregate.add<K>(_capacity);
  aggregate.add<V>(_capacity);
  aggregate.add<Size>(_capacity);
  if (!aggregate.finalize()) {
    return false;
  }

  auto data = m_allocator->allocate(aggregate.bytes());
  if (!data) {
    return false;
  }

  m_keys = reinterpret_cast<K*>(data + aggregate[0]);
  m_values = reinterpret_cast<V*>(data + aggregate[1]);
  m_hashes = reinterpret_cast<Size*>(data + aggregate[2]);

  for (Size i = 0; i < _capacity; i++) {
    element_hash(i) = 0;
  }

  m_capacity = _capacity;
  m_resize_threshold = (m_capacity * LOAD_FACTOR) / 100;
  m_mask = m_capacity - 1;

  return true;
}

template<typename K, typename V>
bool Map<K, V>::grow() {
  const auto old_capacity = m_capacity;
  const auto new_capacity = m_capacity ? m_capacity * 2 : INITIAL_SIZE;

  auto data = m_data;
  auto keys = m_keys;
  auto values = m_values;
  auto hashes = m_hashes;

  if (!allocate(new_capacity)) {
    return false;
  }

  for (Size i = 0; i < old_capacity; i++) {
    const auto hash = hashes[i];
    if (hash == 0 || is_deleted(hash)) {
      continue;
    }

    RX_ASSERT(inserter(hash, Utility::move(keys[i]), Utility::move(values[i])), "insertion failed");

    if constexpr (!Concepts::TriviallyDestructible<K>) {
      Utility::destruct<K>(keys + i);
    }

    if constexpr (!Concepts::TriviallyDestructible<V>) {
      Utility::destruct<V>(values + i);
    }
  }

  m_allocator->deallocate(data);

  return true;
}

template<typename K, typename V>
V* Map<K, V>::construct(Size _index, Size _hash, K&& key_, V&& value_) {
  Utility::construct<K>(m_keys + _index, Utility::forward<K>(key_));
  Utility::construct<V>(m_values + _index, Utility::forward<V>(value_));
  element_hash(_index) = _hash;
  return m_values + _index;
}

template<typename K, typename V>
V* Map<K, V>::inserter(Size _hash, K&& key_, V&& value_) {
  Size position = desired_position(_hash);
  Size distance = 0;

  V* result = nullptr;
  for (;;) {
    if (element_hash(position) == 0) {
      V* insert = construct(position, _hash, Utility::forward<K>(key_), Utility::forward<V>(value_));
      return result ? result : insert;
    }

    const Size existing_element_probe_distance{probe_distance(element_hash(position), position)};
    if (existing_element_probe_distance < distance) {
      if (is_deleted(element_hash(position))) {
        V* insert = construct(position, _hash, Utility::forward<K>(key_), Utility::forward<V>(value_));
        return result ? result : insert;
      }

      if (!result) {
        result = m_values + position;
      }

      Utility::swap(_hash, element_hash(position));
      Utility::swap(key_, m_keys[position]);
      Utility::swap(value_, m_values[position]);

      distance = existing_element_probe_distance;
    }

    position = (position + 1) & m_mask;
    distance++;
  }

  return result;
}

template<typename K, typename V>
V* Map<K, V>::inserter(Size _hash, const K& _key, V&& value_) {
  if (auto key = Utility::copy(_key)) {
    return inserter(_hash, Utility::move(*key), Utility::move(value_));
  }
  return nullptr;
}

template<typename K, typename V>
V* Map<K, V>::inserter(Size _hash, const K& _key, const V& _value) {
  auto key = Utility::copy(_key);
  auto value = Utility::copy(_value);
  if (key && value) {
    return inserter(_hash, Utility::move(*key), Utility::move(*value));
  }
  return nullptr;
}

template<typename K, typename V>
bool Map<K, V>::lookup_index(const K& _key, Size& _index) const {
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

template<typename K, typename V>
template<typename F>
bool Map<K, V>::each_key(F&& _function) {
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

template<typename K, typename V>
template<typename F>
bool Map<K, V>::each_key(F&& _function) const {
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

template<typename K, typename V>
template<typename F>
bool Map<K, V>::each_value(F&& _function) {
  using ReturnType = Traits::InvokeResult<F, V&>;
  for (Size i{0}; i < m_capacity; i++) {
    const auto hash{m_hashes[i]};
    if (hash != 0 && !is_deleted(hash)) {
      if constexpr (Traits::IS_SAME<ReturnType, bool>) {
        if (!_function(m_values[i])) {
          return false;
        }
      } else {
        _function(m_values[i]);
      }
    }
  }
  return true;
}

template<typename K, typename V>
template<typename F>
bool Map<K, V>::each_value(F&& _function) const {
  using ReturnType = Traits::InvokeResult<F, const V&>;
  for (Size i{0}; i < m_capacity; i++) {
    const auto hash{m_hashes[i]};
    if (hash != 0 && !is_deleted(hash)) {
      if constexpr (Traits::IS_SAME<ReturnType, bool>) {
        if (!_function(m_values[i])) {
          return false;
        }
      } else {
        _function(m_values[i]);
      }
    }
  }
  return true;
}

template<typename K, typename V>
template<typename F>
bool Map<K, V>::each_pair(F&& _function) {
  using ReturnType = Traits::InvokeResult<F, const K&, V&>;
  for (Size i{0}; i < m_capacity; i++) {
    const auto hash{m_hashes[i]};
    if (hash != 0 && !is_deleted(hash)) {
      if constexpr (Traits::IS_SAME<ReturnType, bool>) {
        if (!_function(m_keys[i], m_values[i])) {
          return false;
        }
      } else {
        _function(m_keys[i], m_values[i]);
      }
    }
  }
  return true;
}

template<typename K, typename V>
template<typename F>
bool Map<K, V>::each_pair(F&& _function) const {
  using ReturnType = Traits::InvokeResult<F, const K&, const V&>;
  for (Size i{0}; i < m_capacity; i++) {
    const auto hash{m_hashes[i]};
    if (hash != 0 && !is_deleted(hash)) {
      if constexpr (Traits::IS_SAME<ReturnType, bool>) {
        if (!_function(m_keys[i], m_values[i])) {
          return false;
        }
      } else {
        _function(m_keys[i], m_values[i]);
      }
    }
  }
  return true;
}

template<typename K, typename V>
RX_HINT_FORCE_INLINE constexpr Memory::Allocator& Map<K, V>::allocator() const {
  return *m_allocator;
}

} // namespace Rx

#endif // RX_CORE_MAP
