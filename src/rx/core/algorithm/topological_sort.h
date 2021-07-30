#ifndef RX_CORE_ALGORITHM_TOPOLOGICAL_SORT_H
#define RX_CORE_ALGORITHM_TOPOLOGICAL_SORT_H
#include "rx/core/map.h"
#include "rx/core/set.h"
#include "rx/core/vector.h"

/// \file topological_sort.h
namespace Rx::Algorithm {

/// \brief Topological sorter
///
/// O(V + E) generic topological sorter using unordered hashing data structures.
template<typename K>
struct TopologicalSort {
  /// \param _allocator The allocator to use.
  TopologicalSort(Memory::Allocator& _allocator);

  /// Sorted result
  struct Result {
    Vector<K> sorted; ///< Sorted order of nodes.
    Vector<K> cycled; ///< Problem nodes that form cycles.
  };

  /// \brief Add a key to the sorter.
  /// \param _key The key to add
  /// \returns true on success, false otherwise
  /// \note Fails when key has already been added or out of memory.
  [[nodiscard]] bool add(const K& _key);

  /// \brief Add a dependency to the sorter for a given key.
  /// \param _key The key to add a dependency to.
  /// \param _dependency The dependency to add.
  /// \return true on success, false otherwise
  /// \note Fails when the key does not exist, the dependency already
  /// exists for that key, or out of memory.
  [[nodiscard]] bool add(const K& _key, const K& _dependency);

  /// \brief Sort the dependencies.
  /// \return The possibly-sorted \ref Result on success, nullopt when out of memory.
  /// \note The result may not be sorted, check \ref Result::cycled for cycles.
  Optional<Result> sort();

  /// Clear all keys and dependencies for reuse.
  void clear();

  /// The allocator used by the sorter.
  constexpr Memory::Allocator& allocator() const;

private:
  struct Relations {
    RX_MARK_NO_COPY(Relations);

    // Relations();
    Relations(Memory::Allocator& _allocator);
    Relations(Relations&& relations_);
    Relations& operator=(Relations&& relations_);

    constexpr Memory::Allocator& allocator() const;

    static Optional<Relations> copy(const Relations& _relations);

    Size dependencies;
    Set<K> dependents;
  };

  Memory::Allocator* m_allocator;
  Map<K, Relations> m_map;
};

template<typename K>
TopologicalSort<K>::TopologicalSort(Memory::Allocator& _allocator)
  : m_allocator{&_allocator}
  , m_map{allocator()}
{
}

template<typename K>
bool TopologicalSort<K>::add(const K& _key) {
  return m_map.find(_key) || m_map.insert(_key, {allocator()}) != nullptr;
}

template<typename K>
bool TopologicalSort<K>::add(const K& _key, const K& _dependency) {
  // Cannot be a dependency of one-self.
  if (_key == _dependency) {
    return false;
  }

  // Dependents of the dependency.
  {
    auto find = m_map.find(_dependency);
    if (!find && !(find = m_map.insert(_dependency, {allocator()}))) {
      return false;
    }

    auto& dependents = find->dependents;

    // Already a dependency.
    if (dependents.find(_key)) {
      return true;
    }

    if (!dependents.insert(_key)) {
      return false;
    }
  }

  // Dependents of the key.
  {
    auto find = m_map.find(_key);
    if (!find && !(find = m_map.insert(_key, {allocator()}))) {
      return false;
    }

    auto& dependencies = find->dependencies;

    // Another reference for this dependency.
    dependencies++;
  }

  return true;
}

template<typename K>
Optional<typename TopologicalSort<K>::Result> TopologicalSort<K>::sort() {
  // Make a copy of the map because the sorting is destructive.
  auto map = Utility::copy(m_map);
  if (!map) {
    return nullopt;
  }

  Vector<K> sorted{allocator()};
  Vector<K> cycled{allocator()};

  // Each key that has no dependencies can be put in right away.
  auto try_put_key = map->each_pair([&](const K& _key, const Relations& _relations) {
    if (!_relations.dependencies) {
      return sorted.push_back(_key);
    }
    return true;
  });

  if (!try_put_key) {
    return nullopt;
  }

  // Check dependents of the ones with no dependencies and store for each
  // resolved dependency.
  auto try_put_dependents = sorted.each_fwd([&](const K& _root_key) {
    map->find(_root_key)->dependents.each([&](const K& _key) {
      if (!--map->find(_key)->dependencies) {
        return sorted.push_back(_key);
      }
      return true;
    });
  });

  if (!try_put_dependents) {
    return nullopt;
  }

  // When there's remaining dependencies of a relation then we've formed a cycle.
  auto try_put_relations = map->each_pair([&](const K& _key, const Relations& _relations) {
    if (_relations.dependencies) {
      return cycled.push_back(_key);
    }
    return true;
  });

  if (!try_put_relations) {
    return nullopt;
  }

  return Result{Utility::move(sorted), Utility::move(cycled)};
}

template<typename T>
RX_HINT_FORCE_INLINE void TopologicalSort<T>::clear() {
  m_map.clear();
}

template<typename T>
RX_HINT_FORCE_INLINE constexpr Memory::Allocator& TopologicalSort<T>::allocator() const {
  return *m_allocator;
}

// [TopologicalSorter<T>::Relations]
template<typename T>
TopologicalSort<T>::Relations::Relations(Memory::Allocator& _allocator)
  : dependencies{0}
  , dependents{_allocator}
{
}

template<typename T>
TopologicalSort<T>::Relations::Relations(Relations&& relations_)
  : dependencies{Utility::exchange(relations_.dependencies, 0)}
  , dependents{Utility::move(relations_.dependents)}
{
}

template<typename T>
typename TopologicalSort<T>::Relations& TopologicalSort<T>::Relations::operator=(Relations&& relations_) {
  dependencies = Utility::exchange(relations_.dependencies, 0);
  dependents = Utility::move(relations_.dependents);
  return *this;
}

template<typename T>
constexpr typename Memory::Allocator& TopologicalSort<T>::Relations::allocator() const {
  return dependents.allocator();
}

template<typename T>
Optional<typename TopologicalSort<T>::Relations> TopologicalSort<T>::Relations::copy(const Relations& _relations) {
  if (auto dependents = Utility::copy(_relations.dependents)) {
    Relations relations{_relations.allocator()};
    relations.dependencies = _relations.dependencies;
    relations.dependents = Utility::move(*dependents);
    return relations;
  }
  return nullopt;
}

} // namespace Rx::Algorithm

#endif // RX_CORE_ALGORITHM_TOPOLIGCAL_SORT_H
