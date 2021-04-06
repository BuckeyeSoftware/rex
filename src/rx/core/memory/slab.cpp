#include "rx/core/memory/slab.h"
#include "rx/core/memory/aggregate.h"

#include "rx/core/bitset.h"

namespace Rx::Memory {

Slab::Slab(Slab&& slab_)
  : m_caches{Utility::move(slab_.m_caches)}
  , m_object_size{Utility::exchange(slab_.m_object_size, 0)}
  , m_objects_per_cache{Utility::exchange(slab_.m_objects_per_cache, 0)}
  , m_minimum_caches{Utility::exchange(slab_.m_minimum_caches, 0)}
  , m_maximum_caches{Utility::exchange(slab_.m_maximum_caches, 0)}
{
}

Slab& Slab::operator=(Slab&& slab_) {
  if (this != &slab_) {
    release();
    m_caches = Utility::move(slab_.m_caches);
    m_object_size = Utility::exchange(slab_.m_object_size, 0);
    m_objects_per_cache = Utility::exchange(slab_.m_objects_per_cache, 0);
    m_minimum_caches = Utility::exchange(slab_.m_minimum_caches, 0);
    m_maximum_caches = Utility::exchange(slab_.m_maximum_caches, 0);
  }
  return *this;
}

Optional<Slab> Slab::create(Memory::Allocator& _allocator,
  Size _object_size, Size _objects_per_cache, Size _minimum_caches,
  Size _maximum_caches)
{
  // Must provide at least one cache.
  if (_minimum_caches == 0) {
    return nullopt;
  }

  // Round the object size to alignment needed by Rex otherwise the compiler
  // will generate aligned load and store instructions for unaligned data.
  _object_size = Allocator::round_to_alignment(_object_size);

  Vector<Cache> caches{_allocator};
  if (!caches.reserve(_minimum_caches)) {
    return nullopt;
  }

  // Use an aggregate to setup the initial caches. This will be a contiguous
  // allocation representing all the caches from [0, _minimum_caches).
  Memory::Aggregate aggregate;
  for (Size i = 0; i < _minimum_caches; i++) {
    // This can fail when |_object_size| rounded to alignment multiplied by
    // |_objects_per_cache| would overflow.
    if (!aggregate.add(_object_size, Allocator::ALIGNMENT, _objects_per_cache)) {
      return nullopt;
    }
  }
  // Finalization can fail if no caches are added to the aggregate.
  if (!aggregate.finalize()) {
    return nullopt;
  }

  auto data = _allocator.allocate(aggregate.bytes());

  if (!data) {
    return nullopt;
  }

  for (Size i = 0; i < _minimum_caches; i++) {
    auto used = Bitset::create(_allocator, _objects_per_cache);
    if (!used || !caches.emplace_back(data + aggregate[i], Utility::move(*used))) {
      _allocator.deallocate(data);
      return nullopt;
    }
  }

  return Slab {
    Utility::move(caches),
    _object_size,
    _objects_per_cache,
    _minimum_caches,
    _maximum_caches
  };
}

Optional<Size> Slab::cache_index_of(const Byte* _data) const {
  // Check that the allocation is bounded in the cache.
  return m_caches.find_if([&](const Cache& _cache) {
    const auto cache_data = _cache.data;
    return _data >= cache_data
        && _data <= cache_data + m_object_size * (m_objects_per_cache - 1);
  });
}

Byte* Slab::data_of(Size _index) const {
  const auto cache_index = _index / m_objects_per_cache;
  const auto object_index = _index % m_objects_per_cache;
  const auto& cache = m_caches[cache_index];
  return cache.data + m_object_size * object_index;
}

Optional<Size> Slab::allocate_unlocked() {
  const auto n_caches = m_caches.size();

  // The caches from [0, m_minimum_caches) will always be present and never
  // free. Check those first for an available opening.
  for (Size i = 0; i < m_minimum_caches; i++) {
    auto& cache = m_caches[i];
    if (auto index = cache.used.find_first_unset()) {
      // Mark it in use and return the index to it.
      cache.used.set(*index);
      return *index + (m_objects_per_cache * i);
    }
  }

  // Search now from [m_minimum_caches, n_caches) which represent optional
  // caches beyond what the slab must always maintain. These caches may be
  // freed. When encountering a freed cache for the first time, record the
  // position. When an allocation cannot be satisfied by the optional caches,
  // a new allocation for a cache will be made and will attempt to reuse the
  // recorded freed cache index.
  Optional<Size> freed_cache_index;
  for (Size i = m_minimum_caches; i < n_caches; i++) {
    auto& cache = m_caches[i];
    // Cache is allocated.
    if (cache.data) {
      if (auto index = cache.used.find_first_unset()) {
        // Mark it in use and return the index to it.
        cache.used.set(*index);
        return *index + (m_objects_per_cache * i);
      }
    } else if (!freed_cache_index) {
      // Record this freed cache index if one hasn't already been recorded.
      freed_cache_index = i;
    }
  }

  // When no freed cache index is found, a whole new cache will need to be made.
  // Check if doing so would exceed the maximum caches permitted by the slab.
  // Note that |m_maximum_caches| can be zero which indicates there is no upper
  // bound on the number of caches this slab can have.
  if (!freed_cache_index && m_maximum_caches && n_caches == m_maximum_caches) {
    // Cannot make a new cache.
    return nullopt;
  }

  // Create storage for a cache entry.
  auto& allocator = m_caches.allocator();
  auto data = allocator.allocate(m_object_size, m_objects_per_cache);
  if (!data) {
    // Out of memory.
    return nullopt;
  }

  if (freed_cache_index) {
    // Reuse the freed cache by giving it the new storage.
    m_caches[*freed_cache_index].data = data;
  } else {
    // Otherwise create a new cache entry with that storage.
    auto used = Bitset::create(allocator, m_objects_per_cache);
    if (!used || !m_caches.emplace_back(data, Utility::move(*used))) {
      // Out of memory.
      allocator.deallocate(data);
      return nullopt;
    }
  }

  // Try again.
  return allocate_unlocked();
}

void Slab::deallocate_unlocked(Size _cache_index, Size _object_index) {
  auto& cache = m_caches[_cache_index];
  RX_ASSERT(cache.used.test(_object_index), "not allocated");

  cache.used.clear(_object_index);

  // The entire cache is empty.
  //
  // When a cache entry is reused the bitset representing which objects are
  // used will already be cleared out as cache emptiness is determined when
  // the count of set bits is zero.
  //
  // Free the cache allocation if the cache is beyond the minimum amount of
  // caches that must always be active. Since these caches will have higher
  // index than m_minimum_caches, they'll always be individually alllocated.
  if (cache.used.count_set_bits() == 0 && _cache_index > m_minimum_caches) {
    m_caches.allocator().deallocate(cache.data);

    // Mark that the cache is freed.
    cache.data = nullptr;

    // When this happens to be the last cache entry. Remove it completely from
    // the cache list. This is the only time the cache list can be truncated
    // in size for removing caches anywhere else in the list would make
    // indices no longer stable.
    if (_cache_index == m_caches.size() - 1) {
      m_caches.pop_back();
    }
  }
}

void Slab::release() {
  const auto n_caches = m_caches.size();
  if (n_caches == 0) {
    return;
  }

  auto& allocator = m_caches.allocator();

  // Entries from [0, m_minimum_caches) represent a single allocation.
  allocator.deallocate(m_caches[0].data);

  // All the entries beyond [0, m_minimum_caches) represent optional caches
  // that are independently allocated.
  for (Size i = m_minimum_caches; i < n_caches; i++) {
    allocator.deallocate(m_caches[i].data);
  }

  m_caches.clear();
}

} // namespace Rx::Memory