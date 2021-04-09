#include <string.h> // memcpy

#include "rx/core/linear_buffer.h"

namespace Rx {

LinearBuffer::LinearBuffer(LinearBuffer&& buffer_)
  : m_allocator{buffer_.m_allocator}
  , m_data{Utility::exchange(buffer_.m_data, buffer_.m_insitu.data())}
  , m_size{Utility::exchange(buffer_.m_size, 0)}
  , m_capacity{Utility::exchange(buffer_.m_capacity, INSITU_SIZE)}
{
  if (m_data == buffer_.m_insitu.data()) {
    m_data = m_insitu.data();
    memcpy(m_insitu.data(), buffer_.m_insitu.data(), m_size);
  }
}

LinearBuffer& LinearBuffer::operator=(LinearBuffer&& buffer_) {
  if (&buffer_ == this) {
    return *this;
  }

  release();

  m_allocator = buffer_.m_allocator;
  m_data = Utility::exchange(buffer_.m_data, buffer_.m_insitu.data());
  m_size = Utility::exchange(buffer_.m_size, 0);
  m_capacity = Utility::exchange(buffer_.m_capacity, INSITU_SIZE);

  if (m_data == buffer_.m_insitu.data()) {
    m_data = m_insitu.data();
    memcpy(m_insitu.data(), buffer_.m_insitu.data(), m_size);
  }

  return *this;
}

void LinearBuffer::erase(Size _begin, Size _end) {
  RX_ASSERT(_begin < _end, "invalid range");
  RX_ASSERT(_begin < size(), "out of bounds");
  RX_ASSERT(_end < size(), "out of bounds");

  const auto begin = m_data + _begin;
  const auto end = m_data + _end;

  const auto length = (m_data + m_size) - end;

  memmove(begin, end, length);

  m_size -= _end - _begin;
}

bool LinearBuffer::append(const Byte* _data, Size _size) {
  const auto old_size = m_size;
  const auto new_size = m_size + _size;
  if (!resize(new_size)) {
    return false;
  }

  memcpy(m_data + old_size, _data, _size);
  return true;
}

void LinearBuffer::release() {
  if (!in_situ()) {
    m_allocator->deallocate(m_data);
  }
}

bool LinearBuffer::push_back(Byte _value) {
  if (!resize(m_size + 1)) {
    return false;
  }

  m_data[m_size - 1] = _value;
  return true;
}

bool LinearBuffer::reserve(Size _capacity) {
  if (_capacity <= m_capacity) {
    return true;
  }

  // Always resize capacity with the Golden ratio.
  Size capacity = m_capacity;
  while (capacity < _capacity) {
    capacity = ((capacity + 1) * 3) / 2;
  }

  auto data = m_allocator->reallocate(in_situ() ? nullptr : m_data, capacity);
  if (!data) {
    return false;
  }

  // Copy from in-situ.
  if (in_situ()) {
    memcpy(data, m_data, m_size);
  }

  m_data = data;
  m_capacity = capacity;

  return true;
}

bool LinearBuffer::resize(Size _size) {
  if (!reserve(_size)) {
    return false;
  }

  m_size = _size;

  return true;
}

Optional<Memory::View> LinearBuffer::disown() {
  // When in-situ we need to freshly allocate storage.
  if (in_situ()) {
    const auto data = m_allocator->allocate(m_size);
    if (!data) {
      return nullopt;
    }

    const auto size = Utility::exchange(m_size, 0);
    const auto capacity = Utility::exchange(m_capacity, INSITU_SIZE);

    memcpy(data, m_data, size);

    // Reset back to in-situ representation.
    Utility::exchange(m_data, m_insitu.data());

    return Memory::View{m_allocator, data, size, capacity};
  }

  const auto data = Utility::exchange(m_data, m_insitu.data());
  const auto size = Utility::exchange(m_size, 0);
  const auto capacity = Utility::exchange(m_capacity, INSITU_SIZE);

  return Memory::View{m_allocator, data, size, capacity};
}

} // namespace Rx
