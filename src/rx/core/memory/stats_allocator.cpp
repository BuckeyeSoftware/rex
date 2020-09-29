#include "rx/core/memory/stats_allocator.h"
#include "rx/core/concurrency/scope_lock.h"
#include "rx/core/algorithm/max.h"
#include "rx/core/hints/unlikely.h"
#include "rx/core/assert.h"

namespace Rx::Memory {

struct alignas(Allocator::ALIGNMENT) Header {
  Size size;
  Byte* base;
};

Byte* StatsAllocator::allocate(Size _size) {
  const auto rounded_size = round_to_alignment(_size);
  const auto bytes = ALIGNMENT + rounded_size + sizeof(Header);

  auto base = m_allocator.allocate(bytes);
  if (RX_HINT_UNLIKELY(!base)) {
    return nullptr;
  }

  auto header = reinterpret_cast<Header*>(round_to_alignment(base));
  header->size = _size;
  header->base = base;

  auto aligned = reinterpret_cast<Byte*>(header + 1);

  {
    Concurrency::ScopeLock locked{m_lock};
    m_statistics.allocations++;
    m_statistics.used_request_bytes += _size;
    m_statistics.used_actual_bytes += bytes;
    m_statistics.peak_request_bytes = Algorithm::max(m_statistics.peak_request_bytes, m_statistics.used_request_bytes);
    m_statistics.peak_actual_bytes = Algorithm::max(m_statistics.peak_actual_bytes, m_statistics.used_actual_bytes);
  }
  return aligned;
}

Byte* StatsAllocator::reallocate(void* _data, Size _size) {
  if (RX_HINT_UNLIKELY(!_data)) {
    return allocate(_size);
  }

  const auto old_header = reinterpret_cast<Header*>(_data) - 1;
  const auto old_size = old_header->size;
  const auto old_rounded_size = round_to_alignment(old_size);
  const auto old_bytes = ALIGNMENT + old_rounded_size + sizeof(Header);
  const auto old_base = old_header->base;

  const auto new_size = _size;
  const auto new_rounded_size = round_to_alignment(new_size);
  const auto new_bytes = ALIGNMENT + new_rounded_size + sizeof(Header);
  const auto new_base = m_allocator.reallocate(old_base, new_bytes);
  if (RX_HINT_UNLIKELY(!new_base)) {
    return nullptr;
  }

  const auto new_header = reinterpret_cast<Header*>(round_to_alignment(new_base));
  new_header->size = new_size;
  new_header->base = new_base;

  const auto aligned = reinterpret_cast<Byte*>(new_header + 1);

  {
    Concurrency::ScopeLock locked{m_lock};
    m_statistics.request_reallocations++;
    if (new_base == old_base) {
      m_statistics.actual_reallocations++;
    }
    m_statistics.used_request_bytes -= old_size;
    m_statistics.used_actual_bytes -= old_bytes;
    m_statistics.used_request_bytes += new_size;
    m_statistics.used_actual_bytes += new_bytes;
  }
  return aligned;
}

void StatsAllocator::deallocate(void* _data) {
  if (RX_HINT_UNLIKELY(!_data)) {
    return;
  }

  const auto old_header = reinterpret_cast<Header*>(_data) - 1;
  const auto old_size = old_header->size;
  const auto old_rounded_size = round_to_alignment(old_size);
  const auto old_bytes = ALIGNMENT + old_rounded_size + sizeof(Header);
  const auto old_base = old_header->base;

  {
    Concurrency::ScopeLock locked{m_lock};
    m_statistics.deallocations++;
    m_statistics.used_request_bytes -= old_size;
    m_statistics.used_actual_bytes -= old_bytes;
  }

  m_allocator.deallocate(old_base);
}

StatsAllocator::Statistics StatsAllocator::stats() const {
  // Hold a lock and make an entire copy of the structure atomically
  Concurrency::ScopeLock locked{m_lock};
  return m_statistics;
}

} // namespace rx::memory
