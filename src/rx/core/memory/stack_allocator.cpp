#include <string.h> // memcpy

#include <rx/core/memory/stack_allocator.h> // stack_allocator

#include <rx/core/assert.h> // RX_ASSERT
#include <rx/core/traits.h> // move

namespace rx::memory {

stack_allocator::stack_allocator(allocator* base, rx_size size)
  : m_base{base}
  , m_data{base->allocate(size)}
  , m_point{m_data.data()}
{
}

stack_allocator::~stack_allocator() {
  m_base->deallocate(move(m_data));
}

block stack_allocator::allocate(rx_size size) {
  size = round_to_alignment(size);

  if (m_point + size < m_data.end()) {
    const auto point{m_point};
    m_point += size;
    return {size, point};
  }

  return {};
}

block stack_allocator::reallocate(block& old, rx_size size) {
  // reallocate with empty block goes to allocate
  if (!old) {
    return allocate(size);
  }

  size = round_to_alignment(size);

  // no need to resize
  if (old.size() == size) {
    return move(old);
  }

  // inplace resize if last allocated block
  if (old.end() == m_point) {
    if (size < old.size()) {
      // inplace shrink block
      m_point -= old.size() - size;
      return {size, old.data()};
    } else {
      // inplace grow block
      m_point += size - old.size();
      return {size, old.data()};
    }
  }

  // allocate fresh storage and copy into it
  block resize{allocate(size)};
  if (resize) {
    memcpy(resize.data(), old.data(), old.size());
    return move(resize);
  }

  return {};
}

void stack_allocator::deallocate(block&& old) {
  // ensure the passed pointer is inside the memory block allocated
  if (old) {
    RX_ASSERT(owns(old), "block [%p..%p] (%zu) outside heap [%p..%p] (%zu)",
      old.data(), old.end(), old.size(),
      m_data.data(), m_data.end(), m_data.size());

    // when the value passed to us was the last allocated block we can move back
    if (old.end() == m_point) {
      m_point -= old.size();
    }
  }
}

bool stack_allocator::owns(const block& data) const {
  return data.data() >= m_data.data() && data.end() <= m_data.end();
}

} // namespace rx::memory
