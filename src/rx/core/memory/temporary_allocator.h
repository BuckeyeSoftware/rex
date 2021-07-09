#ifndef RX_CORE_MEMORY_TEMPORARY_ALLOCATOR_H
#define RX_CORE_MEMORY_TEMPORARY_ALLOCATOR_H
#include "rx/core/memory/uninitialized_storage.h"
#include "rx/core/memory/buddy_allocator.h"
#include "rx/core/memory/move.h"

#include "rx/core/algorithm/min.h"

namespace Rx::Memory {

/// \brief Temporary allocator.
///
/// An allocator for temporary, short-lived allocations. Provided there is still
/// temporary storage available, allocations will allocate from in-situ storage
/// of size \tparam S. When there is no available storage in-situ, allocations
/// fallback to the allocator given to the constructor.
template<Size S>
struct TemporaryAllocator
  final : Allocator
{
  constexpr TemporaryAllocator(Allocator& _fallback);

  virtual Byte* allocate(Size _size) {
    if (auto attempt = m_buddy.allocate(_size)) {
      return attempt;
    }
    return m_allocator.allocate(_size);
  }

  virtual Byte* reallocate(void* _data, Size _size) {
    auto data = reinterpret_cast<Byte*>(_data);
    // reallocate(nullptr, size) = allocate(size)
    if (!data) {
      return allocate(_size);
    }

    if (is_temporary_allocation(data)) {
      if (auto attempt = m_buddy.reallocate(data, _size)) {
        return attempt;
      } else if (auto attempt = m_allocator.allocate(_size)) {
        // Determine the byte offset in temporary storage of this allocation.
        const auto offset = data - m_storage.data();
        // The amount of bytes remaining after this allocation.
        const auto remain = m_storage.size() - offset;
        // Move which ever is smaller, the bytes given or the remaining bytes
        // after the allocation. This will over-copy the actual original
        // allocation since there is no meta-data of the true size, but this is
        // safe because it's still within the temporary storage, such bytes are
        // considered uninitialized by the caller anyways.
        Memory::move(attempt, data, Algorithm::min(remain, _size));
        m_buddy.deallocate(data);
        return attempt;
      }
    } else if (auto attempt = m_allocator.reallocate(data, _size)) {
      return attempt;
    }
    // Out of memory.
    return nullptr;
  }

  virtual void deallocate(void* _data) {
    if (auto data = reinterpret_cast<Byte*>(_data)) {
      if (is_temporary_allocation(data)) {
        m_buddy.deallocate(data);
      } else {
        m_allocator.deallocate(data);
      }
    }
  }

private:
  bool is_temporary_allocation(const Byte* _data) const {
    return _data >= m_storage.data()
        && _data <= m_storage.data() + m_storage.size();
  }

  UninitializedStorage<S, ALIGNMENT> m_storage;
  BuddyAllocator m_buddy;
  Allocator& m_allocator;
};

template<Size S>
constexpr TemporaryAllocator<S>::TemporaryAllocator(Allocator& _fallback)
  : m_buddy{m_storage.data(), m_storage.size()}
  , m_allocator{_fallback}
{
}

} // namespace Rx::Memory

#endif // RX_CORE_MEMORY_TEMPORARY_ALLOCATOR_H