#include <string.h> // memcpy

#include <rx/core/memory/bump_point_allocator.h>
#include <rx/core/memory/allocator.h>

#include <rx/core/assert.h>

#include <rx/core/utility/move.h>
#include <rx/core/utility/construct.h>

namespace rx::memory {

bump_point_allocator::bump_point_allocator(allocator* _allocator, rx_size _size)
  : m_allocator{_allocator}
  , m_size{allocator::round_to_alignment(_size)}
  , m_data{m_allocator->allocate(m_size)}
  , m_point{m_data}
{
}

bump_point_allocator::~bump_point_allocator() {
  m_allocator->deallocate(m_data);
}

rx_byte* bump_point_allocator::allocate(rx_size _size) {
  _size = allocator::round_to_alignment(_size);

  if (m_point + _size <= m_data + m_size) {
    const auto point{m_point};
    m_point += _size;
    return point;
  }

  return nullptr;
}

void bump_point_allocator::reset() {
  m_point = m_data;
}

} // namespace rx::memory