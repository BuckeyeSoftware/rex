#ifndef RX_CORE_MAP_H
#define RX_CORE_MAP_H

#include <limits.h> // CHAR_BIT
#include <string.h> // memset

#include <rx/core/algorithm.h> // swap
#include <rx/core/hash.h> // hash

#include <rx/core/memory/allocator.h> // allocator

namespace rx {

template<typename K, typename V, typename H = hash<K>>
struct map {
  static constexpr rx_size k_initial_size{256};
  static constexpr rx_size k_load_factor{90};

  map(memory::allocator* alloc);
  ~map();

  template<typename... Ts>
  void emplace(const K& key, Ts&&... args);

  void insert(const K& key, V&& value);
  void insert(const K& key, const V& value);

  V* find(const K& key);
  const V* find(const K& key) const;

  bool erase(const K& key);
  rx_size size() const;

private:
  static rx_size hash_key(const K& key);
  static bool is_deleted(rx_size hash);

  rx_size desired_position(rx_size hash) const;
  rx_size probe_distance(rx_size hash, rx_size slot_index) const;

  rx_size& element_hash(rx_size index);
  rx_size element_hash(rx_size index) const;

  void allocate();
  void grow();

  void construct(rx_size index, rx_size hash, K&& key, V&& value);

  void inserter(rx_size hash, K&& key, V&& value);

  bool lookup_index(const K& key, rx_size& index) const;

  memory::allocator* m_allocator;

  memory::block m_keys;
  memory::block m_values;
  memory::block m_hashes;

  rx_size m_size;
  rx_size m_capacity;
  rx_size m_resize_threshold;
  rx_size m_mask;
};

template<typename K, typename V, typename H>
inline map<K, V, H>::map(memory::allocator* alloc)
  : m_allocator{alloc}
  , m_size{0}
  , m_capacity{k_initial_size}
{
  allocate();
}

template<typename K, typename V, typename H>
inline map<K, V, H>::~map() {
  for (rx_size i{0}; i < m_capacity; i++) {
    if (element_hash(i) != 0) {
      if constexpr (!is_trivially_destructible<K>) {
        call_dtor<K>(m_keys.cast<K*>() + i);
      }
      if constexpr (!is_trivially_destructible<V>) {
        call_dtor<V>(m_values.cast<V*>() + i);
      }
    }
  }
  m_allocator->deallocate(move(m_keys));
  m_allocator->deallocate(move(m_values));
  m_allocator->deallocate(move(m_hashes));
}

template<typename K, typename V, typename H>
template<typename... Ts>
inline void map<K, V, H>::emplace(const K& key, Ts&&... args) {
  if (++m_size >= m_resize_threshold) {
    grow();
  }
  inserter(hash_key(key), move(K{key}), move(V{forward<Ts>(args)...}));
}

template<typename K, typename V, typename H>
inline void map<K, V, H>::insert(const K& key, V&& value) {
  if (++m_size >= m_resize_threshold) {
    grow();
  }
  inserter(hash_key(key), move(K{key}), move(value));
}

template<typename K, typename V, typename H>
inline void map<K, V, H>::insert(const K& key, const V& value) {
  if (++m_size >= m_resize_threshold) {
    grow();
  }
  inserter(hash_key(key), move(K{key}), move(V{value}));
}

template<typename K, typename V, typename H>
V* map<K, V, H>::find(const K& key) {
  const rx_size hash{hash_key(key)};
  if (rx_size index; lookup_index(key, index)) {
    return m_values.cast<V*>() + index;
  }
  return nullptr;
}

template<typename K, typename V, typename H>
const V* map<K, V, H>::find(const K& key) const {
  const rx_size hash{hash_key(key)};
  if (rx_size index; lookup_index(key, index)) {
    return m_values.cast<V*>() + index;
  }
  return nullptr;
}

template<typename K, typename V, typename H>
inline bool map<K, V, H>::erase(const K& key) {
  const rx_size hash{hash_key(key)};
  if (rx_size index; lookup_index(key, index)) {
    if constexpr (!is_trivially_destructible<K>) {
      call_dtor<K>(m_keys.cast<K*>() + index);
    }
    if constexpr (!is_trivially_destructible<V>) {
      call_dtor<V>(m_values.cast<V*>() + index);
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

template<typename K, typename V, typename H>
inline rx_size map<K, V, H>::size() const {
  return m_size;
}

template<typename K, typename V, typename H>
inline rx_size map<K, V, H>::hash_key(const K& key) {
  const H hasher;
  auto hash_value{hasher(key)};

  // MSB is used to indicate deleted elements
  if constexpr(sizeof hash_value == 8) {
    hash_value &= 0x7FFFFFFFFFFFFFFF;
  } else {
    hash_value &= 0x7FFFFFFF;
  }

  // don't ever hash to zero since zero is used to indicate unused slots
  hash_value |= hash_value == 0;

  return hash_value;
}

template<typename K, typename V, typename H>
inline bool map<K, V, H>::is_deleted(rx_size hash) {
  // MSB indicates tombstones
  return (hash >> ((sizeof hash * CHAR_BIT) - 1)) != 0;
}

template<typename K, typename V, typename H>
inline rx_size map<K, V, H>::desired_position(rx_size hash) const {
  return hash & m_mask;
}

template<typename K, typename V, typename H>
inline rx_size map<K, V, H>::probe_distance(rx_size hash, rx_size slot_index) const {
  return (slot_index + m_capacity - desired_position(hash)) & m_mask;
}

template<typename K, typename V, typename H>
inline rx_size& map<K, V, H>::element_hash(rx_size index) {
  return m_hashes.cast<rx_size*>()[index];
}

template<typename K, typename V, typename H>
inline rx_size map<K, V, H>::element_hash(rx_size index) const {
  return m_hashes.cast<const rx_size*>()[index];
}

template<typename K, typename V, typename H>
inline void map<K, V, H>::allocate() {
  m_keys = move(m_allocator->allocate(sizeof(K) * m_capacity));
  m_values = move(m_allocator->allocate(sizeof(V) * m_capacity));
  m_hashes = move(m_allocator->allocate(sizeof(rx_size) * m_capacity));

  for (rx_size i{0}; i < m_capacity; i++) {
    element_hash(i) = 0;
  }

  m_resize_threshold = (m_capacity * k_load_factor) / 100;
  m_mask = m_capacity - 1;
}

template<typename K, typename V, typename H>
inline void map<K, V, H>::grow() {
  const auto old_capacity{m_capacity};

  auto keys{move(m_keys)};
  auto values{move(m_values)};
  auto hashes{move(m_hashes)};

  m_capacity *= 2;
  allocate();

  auto keys_data{keys.template cast<K*>()};
  auto values_data{values.template cast<V*>()};
  auto hashes_data{hashes.template cast<rx_size*>()};

  for (rx_size i{0}; i < old_capacity; i++) {
    const auto hash{hashes_data[i]};
    if (hash != 0 && !is_deleted(hash)) {
      inserter(hash, move(keys_data[i]), move(values_data[i]));
      if constexpr (!is_trivially_destructible<K>) {
        call_dtor<K>(keys_data + i);
      }
      if constexpr (!is_trivially_destructible<V>) {
        call_dtor<V>(values_data + i);
      }
    }
  }

  m_allocator->deallocate(move(keys));
  m_allocator->deallocate(move(values));
  m_allocator->deallocate(move(hashes));
}

template<typename K, typename V, typename H>
inline void map<K, V, H>::construct(rx_size index, rx_size hash, K&& key, V&& value) {
  call_ctor<K>(m_keys.cast<K*>() + index, move(key));
  call_ctor<V>(m_values.cast<V*>() + index, move(value));
  element_hash(index) = hash;
}

template<typename K, typename V, typename H>
inline void map<K, V, H>::inserter(rx_size hash, K&& key, V&& value) {
  rx_size position{desired_position(hash)};
  rx_size distance{0};
  for (;;) {
    if (element_hash(position) == 0) {
      construct(position, hash, move(key), move(value));
      return;
    }

    const rx_size existing_element_probe_distance{probe_distance(element_hash(position), position)};
    if (existing_element_probe_distance < distance) {
      if (is_deleted(element_hash(position))) {
        construct(position, hash, move(key), move(value));
        return;
      }

      swap(hash, element_hash(position));
      swap(key, m_keys.cast<K*>()[position]);
      swap(value, m_values.cast<V*>()[position]);

      distance = existing_element_probe_distance;
    }

    position = (position + 1) & m_mask;
    distance++;
  }
}

template<typename K, typename V, typename H>
inline bool map<K, V, H>::lookup_index(const K& key, rx_size& index) const {
  const rx_size hash{hash_key(key)};
  rx_size position{desired_position(hash)};
  rx_size distance{0};
  for (;;) {
    if (element_hash(position) == 0) {
      return false;
    } else if (distance > probe_distance(element_hash(position), position)) {
      return false;
    } else if (element_hash(position) == hash && m_keys.cast<K*>()[position] == key) {
      index = position;
      return true;
    }
    position = (position + 1) & m_mask;
    distance++;
  }
  return false; // unreachable
}

} // namespace rx

#endif // RX_CORE_MAP_H
