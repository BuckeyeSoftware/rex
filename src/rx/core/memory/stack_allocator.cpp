#include <string.h> // memcpy

#include <rx/core/memory/stack_allocator.h> // stack_allocator

#include <rx/core/assert.h> // RX_ASSERT

#include <rx/core/utility/move.h>
#include <rx/core/utility/construct.h>

namespace rx::memory {

stack_allocator::stack_allocator(allocator* _allocator, rx_size _size)
  : m_allocator{_allocator}
  , m_data{m_allocator->allocate(_size)}
  , m_size{_size}
  , m_point{m_data}
{
}

stack_allocator::~stack_allocator() {
  m_allocator->deallocate(m_data);
}

rx_byte* stack_allocator::allocate(rx_size _size) {
  _size = round_to_alignment(_size);

  if (m_point + _size <= m_data + m_size) {
    const auto point{m_point};
    m_point += _size;
    return point;
  }

  return {};
}

rx_byte* stack_allocator::reallocate(rx_byte*, rx_size _size) {
  // TODO header
  return allocate(_size);
}

void stack_allocator::deallocate(rx_byte*) {
  // TODO header
}

bool stack_allocator::owns(const rx_byte* _data) const {
  return _data >= m_data && _data <= m_data + m_size;
}

void stack_allocator::reset() {
  m_point = m_data;
}

} // namespace rx::memory