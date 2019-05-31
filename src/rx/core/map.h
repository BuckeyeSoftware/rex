#ifndef RX_CORE_MAP_H
#define RX_CORE_MAP_H

#include "rx/core/hash.h" // hash

#include "rx/core/utility/construct.h"
#include "rx/core/utility/destruct.h"
#include "rx/core/utility/move.h"
#include "rx/core/utility/swap.h"

#include "rx/core/traits/return_type.h"
#include "rx/core/traits/is_same.h"

#include "rx/core/memory/system_allocator.h" // allocator, g_system_allocator

namespace rx {

// 32-bit: 28 bytes
// 64-bit: 56 bytes
template<typename K, typename V>
struct map {
  static constexpr rx_size k_initial_size{256};
  static constexpr rx_size k_load_factor{90};

  map();
  map(memory::allocator* _allocator);
  map(map&& _map);
  map(const map& _map);
  ~map();

  map& operator=(map&& _map);
  map& operator=(const map& _map);

  void insert(const K& _key, V&& _value);
  void insert(const K& _key, const V& _value);

  V* find(const K& _key);
  const V* find(const K& _key) const;

  bool erase(const K& _key);
  rx_size size() const;
  bool is_empty() const;

  void clear();

  template<typename F>
  bool each(F&& _function);

  template<typename F>
  bool each(F&& _function) const;

  memory::allocator* allocator() const;

private:
  void clear_and_deallocate();
  void initialize(memory::allocator* _allocator, rx_size _capacity);

  static rx_size hash_key(const K& _key);
  static bool is_deleted(rx_size _hash);

  rx_size desired_position(rx_size _hash) const;
  rx_size probe_distance(rx_size _hash, rx_size _slot_index) const;

  rx_size& element_hash(rx_size index);
  rx_size element_hash(rx_size index) const;

  void allocate();
  void grow();

  // move and non-move construction functions
  void construct(rx_size _index, rx_size _hash, K&& _key, V&& _value);

  void inserter(rx_size _hash, K&& _key, V&& _value);
  void inserter(rx_size _hash, const K& _key, const V& _value);
  void inserter(rx_size _hash, const K& _key, V&& _value);

  bool lookup_index(const K& _key, rx_size& _index) const;

  memory::allocator* m_allocator;

  K* m_keys;
  V* m_values;
  rx_size* m_hashes;

  rx_size m_size;
  rx_size m_capacity;
  rx_size m_resize_threshold;
  rx_size m_mask;
};

template<typename K, typename V>
inline map<K, V>::map()
  : map{&memory::g_system_allocator}
{
}

template<typename K, typename V>
inline map<K, V>::map(memory::allocator* _allocator)
{
  initialize(_allocator, k_initial_size);
  allocate();
}

template<typename K, typename V>
inline map<K, V>::map(map&& _map)
  : m_allocator{_map.m_allocator}
  , m_keys{_map.m_keys}
  , m_values{_map.m_values}
  , m_hashes{_map.m_hashes}
  , m_size{_map.m_size}
  , m_capacity{_map.m_capacity}
  , m_resize_threshold{_map.m_resize_threshold}
  , m_mask{_map.m_mask}
{
  _map.initialize(&memory::g_system_allocator, 0);
}

template<typename K, typename V>
inline map<K, V>::map(const map& _map)
  : map{_map.m_allocator}
{
  for (rx_size i{0}; i < _map.m_capacity; i++) {
    if (_map.element_hash(i) != 0) {
      insert(_map.m_keys[i], _map.m_values[i]);
    }
  }
}

template<typename K, typename V>
inline map<K, V>::~map() {
  clear_and_deallocate();
}

template<typename K, typename V>
inline void map<K, V>::clear() {
  for (rx_size i{0}; i < m_capacity; i++) {
    if (element_hash(i) != 0) {
      if constexpr (!traits::is_trivially_destructible<K>) {
        utility::destruct<K>(m_keys + i);
      }
      if constexpr (!traits::is_trivially_destructible<V>) {
        utility::destruct<V>(m_values + i);
      }
    }
  }
  m_size = 0;
}

template<typename K, typename V>
inline void map<K, V>::clear_and_deallocate() {
  clear();

  m_allocator->deallocate(reinterpret_cast<rx_byte*>(m_keys));
  m_allocator->deallocate(reinterpret_cast<rx_byte*>(m_values));
  m_allocator->deallocate(reinterpret_cast<rx_byte*>(m_hashes));
}

template<typename K, typename V>
inline map<K, V>& map<K, V>::operator=(map<K, V>&& _map) {
  clear_and_deallocate();

  m_allocator = _map.m_allocator;
  m_keys = _map.m_keys;
  m_values = _map.m_values;
  m_hashes = _map.m_hashes;
  m_size = _map.m_size;
  m_capacity = _map.m_capacity;
  m_resize_threshold = _map.m_resize_threshold;
  m_mask = _map.m_mask;

  _map.initialize(&memory::g_system_allocator, 0);

  return *this;
}

template<typename K, typename V>
inline map<K, V>& map<K, V>::operator=(const map<K, V>& _map) {
  clear_and_deallocate();
  initialize(_map.m_allocator, _map.m_capacity);
  allocate();

  for (rx_size i{0}; i < _map.m_capacity; i++) {
    if (_map.element_hash(i) != 0) {
      insert(_map.m_keys[i], _map.m_values[i]);
    }
  }

  return *this;
}

template<typename K, typename V>
inline void map<K, V>::initialize(memory::allocator* _allocator, rx_size _capacity) {
  m_allocator = _allocator;
  m_keys = nullptr;
  m_values = nullptr;
  m_hashes = nullptr;
  m_size = 0;
  m_capacity = _capacity;
  m_resize_threshold = 0;
  m_mask = 0;
}

template<typename K, typename V>
inline void map<K, V>::insert(const K& _key, V&& _value) {
  if (++m_size >= m_resize_threshold) {
    grow();
  }
  inserter(hash_key(_key), _key, utility::move(_value));
}

template<typename K, typename V>
inline void map<K, V>::insert(const K& _key, const V& _value) {
  if (++m_size >= m_resize_threshold) {
    grow();
  }
  inserter(hash_key(_key), _key, _value);
}

template<typename K, typename V>
V* map<K, V>::find(const K& _key) {
  if (rx_size index; lookup_index(_key, index)) {
    return m_values + index;
  }
  return nullptr;
}

template<typename K, typename V>
const V* map<K, V>::find(const K& _key) const {
  if (rx_size index; lookup_index(_key, index)) {
    return m_values + index;
  }
  return nullptr;
}

template<typename K, typename V>
inline bool map<K, V>::erase(const K& _key) {
  if (rx_size index; lookup_index(_key, index)) {
    if constexpr (!traits::is_trivially_destructible<K>) {
      utility::destruct<K>(m_keys + index);
    }
    if constexpr (!traits::is_trivially_destructible<V>) {
      utility::destruct<V>(m_values + index);
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
inline rx_size map<K, V>::size() const {
  return m_size;
}

template<typename K, typename V>
inline bool map<K, V>::is_empty() const {
  return m_size == 0;
}

template<typename K, typename V>
inline rx_size map<K, V>::hash_key(const K& _key) {
  auto hash_value{hash<K>{}(_key)};

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

template<typename K, typename V>
inline bool map<K, V>::is_deleted(rx_size _hash) {
  // MSB indicates tombstones
  return (_hash >> ((sizeof _hash * 8) - 1)) != 0;
}

template<typename K, typename V>
inline rx_size map<K, V>::desired_position(rx_size _hash) const {
  return _hash & m_mask;
}

template<typename K, typename V>
inline rx_size map<K, V>::probe_distance(rx_size _hash, rx_size _slot_index) const {
  return (_slot_index + m_capacity - desired_position(_hash)) & m_mask;
}

template<typename K, typename V>
inline rx_size& map<K, V>::element_hash(rx_size _index) {
  return m_hashes[_index];
}

template<typename K, typename V>
inline rx_size map<K, V>::element_hash(rx_size _index) const {
  return m_hashes[_index];
}

template<typename K, typename V>
inline void map<K, V>::allocate() {
  m_keys = reinterpret_cast<K*>(m_allocator->allocate(sizeof(K) * m_capacity));
  m_values = reinterpret_cast<V*>(m_allocator->allocate(sizeof(V) * m_capacity));
  m_hashes = reinterpret_cast<rx_size*>(m_allocator->allocate(sizeof(rx_size) * m_capacity));

  for (rx_size i{0}; i < m_capacity; i++) {
    element_hash(i) = 0;
  }

  m_resize_threshold = (m_capacity * k_load_factor) / 100;
  m_mask = m_capacity - 1;
}

template<typename K, typename V>
inline void map<K, V>::grow() {
  const auto old_capacity{m_capacity};

  auto keys_data{m_keys};
  auto values_data{m_values};
  auto hashes_data{m_hashes};

  m_capacity *= 2;
  allocate();

  for (rx_size i{0}; i < old_capacity; i++) {
    const auto hash{hashes_data[i]};
    if (hash != 0 && !is_deleted(hash)) {
      inserter(hash, utility::move(keys_data[i]), utility::move(values_data[i]));
      if constexpr (!traits::is_trivially_destructible<K>) {
        utility::destruct<K>(keys_data + i);
      }
      if constexpr (!traits::is_trivially_destructible<V>) {
        utility::destruct<V>(values_data + i);
      }
    }
  }

  m_allocator->deallocate(reinterpret_cast<rx_byte*>(keys_data));
  m_allocator->deallocate(reinterpret_cast<rx_byte*>(values_data));
  m_allocator->deallocate(reinterpret_cast<rx_byte*>(hashes_data));
}

template<typename K, typename V>
inline void map<K, V>::construct(rx_size _index, rx_size _hash, K&& _key, V&& _value) {
  utility::construct<K>(m_keys + _index, utility::move(_key));
  utility::construct<V>(m_values + _index, utility::move(_value));
  element_hash(_index) = _hash;
}

template<typename K, typename V>
inline void map<K, V>::inserter(rx_size _hash, K&& _key, V&& _value) {
  rx_size position{desired_position(_hash)};
  rx_size distance{0};
  for (;;) {
    if (element_hash(position) == 0) {
      construct(position, _hash, utility::move(_key), utility::move(_value));
      return;
    }

    const rx_size existing_element_probe_distance{probe_distance(element_hash(position), position)};
    if (existing_element_probe_distance < distance) {
      if (is_deleted(element_hash(position))) {
        construct(position, _hash, utility::move(_key), utility::move(_value));
        return;
      }

      utility::swap(_hash, element_hash(position));
      utility::swap(_key, m_keys[position]);
      utility::swap(_value, m_values[position]);

      distance = existing_element_probe_distance;
    }

    position = (position + 1) & m_mask;
    distance++;
  }
}

template<typename K, typename V>
inline void map<K, V>::inserter(rx_size _hash, const K& _key, V&& _value) {
  K key{_key};
  return inserter(_hash, utility::move(key), utility::move(_value));
}

template<typename K, typename V>
inline void map<K, V>::inserter(rx_size _hash, const K& _key, const V& _value) {
  K key{_key};
  V value{_value};
  return inserter(_hash, utility::move(key), utility::move(value));
}

template<typename K, typename V>
inline bool map<K, V>::lookup_index(const K& _key, rx_size& _index) const {
  const rx_size hash{hash_key(_key)};
  rx_size position{desired_position(hash)};
  rx_size distance{0};
  for (;;) {
    if (element_hash(position) == 0) {
      return false;
    } else if (distance > probe_distance(element_hash(position), position)) {
      return false;
    } else if (element_hash(position) == hash && m_keys[position] == _key) {
      _index = position;
      return true;
    }
    position = (position + 1) & m_mask;
    distance++;
  }
  RX_UNREACHABLE();
}

template<typename K, typename V>
template<typename F>
inline bool map<K, V>::each(F&& _function) {
  for (rx_size i{0}; i < m_capacity; i++) {
    const auto hash{m_hashes[i]};
    if (hash != 0 && !is_deleted(hash)) {
      if constexpr (traits::is_same<traits::return_type<F>, bool>) {
        if (!_function(m_hashes[i], m_keys[i], m_values[i])) {
          return false;
        }
      } else {
        _function(m_hashes[i], m_keys[i], m_values[i]);
      }
    }
  }
  return true;
}

template<typename K, typename V>
inline memory::allocator* map<K, V>::allocator() const {
  return m_allocator;
}

template<typename K, typename V>
template<typename F>
inline bool map<K, V>::each(F&& _function) const {
  for (rx_size i{0}; i < m_capacity; i++) {
    const auto hash{m_hashes[i]};
    if (hash != 0 && !is_deleted(hash)) {
      if constexpr (traits::is_same<traits::return_type<F>, bool>) {
        if (!_function(m_hashes[i], m_keys[i], m_values[i])) {
          return false;
        }
      } else {
        _function(m_hashes[i], m_keys[i], m_values[i]);
      }
    }
  }
  return true;
}

} // namespace rx

#endif // RX_CORE_MAP
