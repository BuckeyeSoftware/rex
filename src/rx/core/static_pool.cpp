#include "rx/core/static_pool.h"

#include "rx/core/utility/exchange.h"

namespace Rx {

Optional<StaticPool> StaticPool::create(Memory::Allocator& _allocator,
  Size _object_size, Size _object_count)
{
  auto object_size = Memory::Allocator::round_to_alignment(_object_size);
  auto object_count = _object_count;

  auto bitset = Bitset::create(_allocator, object_count);
  if (!bitset) {
    return nullopt;
  }

  auto data = _allocator.allocate(object_size, object_count);
  if (!data) {
    return nullopt;
  }

  return StaticPool{_allocator, object_size, object_count, data, Utility::move(*bitset)};
}

StaticPool::StaticPool(StaticPool&& pool_)
  : m_allocator{pool_.m_allocator}
  , m_object_size{Utility::exchange(pool_.m_object_size, 0)}
  , m_object_count{Utility::exchange(pool_.m_object_count, 0)}
  , m_data{Utility::exchange(pool_.m_data, nullptr)}
  , m_bitset{Utility::move(pool_.m_bitset)}
{
}

StaticPool& StaticPool::operator=(StaticPool&& pool_) {
  if (&pool_ != this) {
    m_allocator = pool_.m_allocator;
    m_object_size = Utility::exchange(pool_.m_object_size, 0);
    m_object_count = Utility::exchange(pool_.m_object_count, 0);
    m_data = Utility::exchange(pool_.m_data, nullptr);
    m_bitset = Utility::move(pool_.m_bitset);
  }
  return *this;
}

Optional<Size> StaticPool::allocate() {
  const auto index = m_bitset.find_first_unset();
  if (RX_HINT_UNLIKELY(!index)) {
    return nullopt;
  }

  m_bitset.set(*index);
  return *index;
}

void StaticPool::deallocate(Size _index) {
  RX_ASSERT(m_bitset.test(_index), "unallocated");
  m_bitset.clear(_index);
}

} // namespace Rx
