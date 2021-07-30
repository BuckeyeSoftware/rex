#ifndef RX_CORE_MEMORY_SLAB_H
#define RX_CORE_MEMORY_SLAB_H
#include "rx/core/vector.h"
#include "rx/core/bitset.h"

#include "rx/core/concurrency/mutex.h"
#include "rx/core/concurrency/scope_lock.h"

namespace Rx::Memory {

/// \brief A slab allocator
///
/// Slab allocation maintains a fixed-capacity cache of objects of the same
/// size similar to a pool. Allocations search for a free slot in the cache
/// and use it. The difference between a slab and a pool comes down to what
/// happens when no more slots are available. When no slots are available, the
/// slab allocates another cache of the same fixed-capacity for objects of the
/// same size and uses it. The result is a series of caches.
///
/// The slab is constructed with a minimum count of caches. These are caches
/// that will always be present in the slab. Optionally, the slab can be
/// constrained to have a maximum count of caches. When the slab runs out of
/// caches it'll allocate another, provided doing so won't exceed the maximum
/// count set. Since the minimum count of caches are always present in the slab,
/// they are allocated contiguously affording some additional cache efficiency
/// as a well-behaved slab will produce non-framented memory addresses.
///
/// The difficulty in using a slab effectively is picking the right amount of
/// objects per cache and the initial minimum cache count. Too many objects in a
/// cache and the most recently added cache will waste a lot of memory and
/// potentially never get close to full capacity. Too few objects per cache and
/// caches will be allocated often, fragmenting memory, and lead to slower
/// operations as more caches need to be searched. Setting too high of an
/// initial minimum cache count will waste memory. Setting too low of an initial
/// minimum cache count will lead to thrashing of the slab as it rushes to
/// create new caches. Similarly, when a cache becomes entirely free and the
/// number of caches in the slab is larger than the minimum cache count, the
/// slab will attempt to remove the cache to reduce memory usage. This removal
/// can be triggered often with low initial minimum cache count too, leading to
/// suboptimal performance.
///
/// Setting the maximum cache count to the same as the minimum cache count
/// models an object pool.
struct Slab {
  Slab();
  Slab(Slab&& slab_);
  ~Slab();
  Slab& operator=(Slab&& slab_);

  /// Create a slab
  /// \param _allocator The allocator to use.
  /// \param _object_size The size of the object that will be allocated.
  /// \param _objects_per_cache The number of objects each cache will hold.
  /// \param _minimum_caches The minimum number of caches to always maintain.
  /// \param _maximum_caches The maximum number of caches permitted.
  /// \note A value of 0 for \p _maximum_caches disables cache limit.
  static Optional<Slab> create(Memory::Allocator& _allocator,
    Size _object_size, Size _objects_per_cache, Size _minimum_caches,
    Size _maximum_caches = 0);

  /// Create an object with the slab.
  /// \warning sizeof(T) must be <= \p _object_size.
  template<typename T, typename... Ts>
  T* create(Ts&&... _arguments);

  /// Destroy an object.
  template<typename T>
  void destroy(T* _data);

  /// Clear caches.
  void clear();

  Size capacity() const;
  Size size() const;

private:
  // When a cache is empty and it's beyond the minimum caches that a slab must
  // maintain, it's data is freed but the bitset representing which objects in
  // the cache is not. A freed cache is reused by allocating fresh storage for
  // the cache but reusing this bitset.
  struct Cache {
    RX_MARK_NO_COPY(Cache);
    constexpr Cache();
    Cache(Byte* _data, Bitset&& bitset_);
    Cache(Cache&& cache_);
    Byte* data;
    Bitset used;
  };

  Slab(Vector<Cache>&& caches_, Size _object_size, Size _objects_per_cache,
    Size _minimum_caches, Size _maximum_caches);

  Optional<Size> cache_index_of(const Byte* _data) const;
  Byte* data_of(Size _index) const;

  Optional<Size> allocate_unlocked();
  void deallocate_unlocked(Size _cache_index, Size _object_index);

  void release();

  mutable Concurrency::Mutex m_mutex;
  Vector<Cache> m_caches RX_HINT_GUARDED_BY(m_mutex);
  Size m_object_size;
  Size m_objects_per_cache;
  Size m_minimum_caches;
  Size m_maximum_caches;
};

// [Slab::Cache]
inline constexpr Slab::Cache::Cache()
  : data{nullptr}
{
}

inline Slab::Cache::Cache(Byte* _data, Bitset&& bitset_)
  : data{_data}
  , used{Utility::move(bitset_)}
{
}

inline Slab::Cache::Cache(Cache&& cache_)
  : data{Utility::exchange(cache_.data, nullptr)}
  , used{Utility::move(cache_.used)}
{
}

// [Slab]
inline Slab::Slab()
  : m_object_size{0}
  , m_objects_per_cache{0}
  , m_minimum_caches{0}
  , m_maximum_caches{0}
{
}

inline Slab::Slab(Vector<Cache>&& caches_, Size _object_size,
  Size _objects_per_cache, Size _minimum_caches, Size _maximum_caches)
  : m_caches{Utility::move(caches_)}
  , m_object_size{_object_size}
  , m_objects_per_cache{_objects_per_cache}
  , m_minimum_caches{_minimum_caches}
  , m_maximum_caches{_maximum_caches}
{
}

inline Slab::~Slab() {
  release();
}

template<typename T, typename... Ts>
T* Slab::create(Ts&&... _arguments) {
  // Object would be too big to fit.
  if (sizeof(T) > m_object_size) {
    return nullptr;
  }

  Concurrency::ScopeLock lock{m_mutex};
  if (const auto index = allocate_unlocked()) {
    return Utility::construct<T>(data_of(*index), Utility::forward<Ts>(_arguments)...);
  }

  return nullptr;
}

template<typename T>
void Slab::destroy(T* _data) {
  Concurrency::ScopeLock lock{m_mutex};
  const auto data = reinterpret_cast<Byte*>(_data);
  const auto cache_index = cache_index_of(data);
  RX_ASSERT(cache_index, "not part of this slab");
  Utility::destruct<T>(data);
  const auto object_index = (data - m_caches[*cache_index].data) / m_object_size;
  deallocate_unlocked(*cache_index, object_index);
}

inline Size Slab::capacity() const {
  Concurrency::ScopeLock lock{m_mutex};

  // Count only non-free caches.
  Size count = 0;
  m_caches.each_fwd([&count](const Cache& _cache) {
    if (_cache.data) {
      count++;
    }
  });

  return count * m_objects_per_cache;
}

inline Size Slab::size() const {
  Concurrency::ScopeLock lock{m_mutex};

  // Count set bits in each cache.
  Size count = 0;
  m_caches.each_fwd([&count](const Cache& _cache) {
    count += _cache.used.count_set_bits();
  });

  return count;
}

} // namespace Rx::Memory

#endif // RX_CORE_MEMORY_SLAB_H