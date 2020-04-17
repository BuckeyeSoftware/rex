#include <string.h> // memcpy

#include "rx/core/memory/buddy_allocator.h"

#include "rx/core/concurrency/scope_lock.h"

#include "rx/core/hints/unlikely.h"
#include "rx/core/hints/likely.h"

#include "rx/core/assert.h"

namespace rx::memory {

// Each allocation in the heap is prefixed with this header.
struct alignas(allocator::k_alignment) block {
  rx_size size;
  bool free;
};

// Returns the next block in the intrusive, flat linked-list structure.
static inline block* next(block* _block) {
  return reinterpret_cast<block*>(reinterpret_cast<rx_byte*>(_block) + _block->size);
}

// Returns size that is needed for |_size|.
static inline rx_size needed(rx_size _size) {
  rx_size result = allocator::k_alignment; // Smallest allocation

  // Storage for the block.
  _size += sizeof(block);

  // Continually double result until |_size| fits.
  while (_size > result) {
    result <<= 1;
  }

  return result;
}

// Continually divides the block |block_| until it's the optimal size for
// an allocation of size |_size|.
static block* divide(block* block_, rx_size _size) {
  while (block_->size > _size) {
    // Split block into two halves, half-size each.
    const auto size = block_->size >> 1;
    block_->size = size;

    block_ = next(block_);
    block_->size = size;
    block_->free = true;
  }

  return block_->size >= _size ? block_ : nullptr;
}

// Searches for a free block that matches the given size |_size| in the list
// defined by |_head| and |_tail|. When a block cannot be found which satisifies
// the size |_size| but there is a larger free block, this divides the free
// block into two halves of the same size until the block optimally fits the
// size |_size| in it. If there is no larger free block available, this returns
// nullptr.
//
// This function also merges adjacent free blocks as it searches to make larger,
// free blocks available during the search.
static block* find_available(block* _head, block* _tail, rx_size _size) {
  block* region = _head;
  block* buddy = next(region);
  block* closest = nullptr;

  // When at the end of the heap and the region is free.
  if (buddy == _tail && region->free) {
    // Split it into a block to satisfy the request leaving what is left over
    // for any future allocations. This is the one edge case the general
    // algorithm cannot cover.
    return divide(region, _size);
  }

  // Find the closest minimum sized match within the heap.
  rx_size closest_size = 0;
  while (region < _tail && buddy < _tail) {
    // When both the region and the buddy are free, merge those adjacent
    // free blocks.
    if (region->free && buddy->free && region->size == buddy->size) {
      region->size <<= 1;

      const rx_size region_size = region->size;
      if (_size <= region_size && (!closest || region_size <= closest->size)) {
        closest = region;
      }

      if ((region = next(buddy)) < _tail) {
        buddy = next(region);
      }
    } else {
      if (closest) {
        closest_size = closest->size;
      }

      // Check the region block.
      const rx_size region_size = region->size;
      if (region->free && _size <= region_size
        && (!closest || region_size <= closest->size))
      {
        closest = region;
        closest_size = region_size;
      }

      // Check the buddy block.
      const rx_size buddy_size = buddy->size;
      if (buddy->free && _size <= buddy_size
        && (!closest || buddy_size <= closest->size))
      {
        closest = buddy;
        closest_size = buddy_size;
      }

      // The buddy has been split up into smaller blocks.
      if (region_size > buddy_size) {
        region = buddy;
        buddy = next(buddy);
      } else if ((region = next(buddy)) < _tail) {
        // Skip the base and buddy region for the next iteration.
        buddy = next(region);
      }
    }
  }

  if (closest) {
    // Perfect match.
    if (closest_size == _size) {
      return closest;
    }

    // Split |current| in halves continually until it optimally fits |_size|.
    return divide(closest, _size);
  }

  // Potentially out of memory.
  return nullptr;
}

// Performs a single level merge of adjacent free blocks in the list given
// by |_head| and |_tail|.
static bool merge_free(block* _head, block* _tail) {
  block* region = _head;
  block* buddy = next(region);

  bool modified = false;
  while (region < _tail && buddy < _tail) {
    // Both the region and buddy are free.
    if (region->free && buddy->free && region->size == buddy->size) {
      // Merge the blocks back into one, larger one.
      region->size <<= 1;
      if ((region = next(region)) < _tail) {
        buddy = next(region);
      }
      modified = true;
    } else if (region->size > buddy->size) {
      // The buddy block has been split into smaller blocks.
      region = buddy;
      buddy = next(buddy);
    } else if ((region = next(buddy)) < _tail) {
      // Skip the base and buddy region for the next iteration.
      buddy = next(region);
    }
  }

  return modified;
}

buddy_allocator::buddy_allocator(rx_byte* _data, rx_size _size) {
  // Ensure |_data| and |_size| are multiples of |k_alignment|.
  RX_ASSERT(reinterpret_cast<rx_uintptr>(_data) % k_alignment == 0,
    "_data not a multiple of k_alignment");
  RX_ASSERT(_size % k_alignment == 0,
    "_size not a multiple of k_alignment");

  // Ensure |_size| is a power of two.
  RX_ASSERT((_size & (_size - 1)) == 0, "_size not a power of two");

  // Create the root block structure.
  block* head = reinterpret_cast<block*>(_data);

  head->size = _size;
  head->free = true;

  m_head = reinterpret_cast<void*>(head);
  m_tail = reinterpret_cast<void*>(next(head));
}

rx_byte* buddy_allocator::allocate(rx_size _size) {
  concurrency::scope_lock lock{m_lock};
  return allocate_unlocked(_size);
}

rx_byte* buddy_allocator::reallocate(void* _data, rx_size _size) {
  concurrency::scope_lock lock{m_lock};
  return reallocate_unlocked(_data, _size);
}

void buddy_allocator::deallocate(void* _data) {
  concurrency::scope_lock lock{m_lock};
  deallocate_unlocked(_data);
}

rx_byte* buddy_allocator::allocate_unlocked(rx_size _size) {
  const auto size = needed(_size);

  const auto head = reinterpret_cast<block*>(m_head);
  const auto tail = reinterpret_cast<block*>(m_tail);

  auto find = find_available(head, tail, size);

  if (RX_HINT_LIKELY(find)) {
    find->free = false;
    return reinterpret_cast<rx_byte*>(find + 1);
  }

  // Merge free blocks until they're all merged.
  while (merge_free(head, tail)) {
    ;
  }

  // Search again for a free block.
  if ((find = find_available(head, tail, size))) {
    find->free = false;
    return reinterpret_cast<rx_byte*>(find + 1);
  }

  // Out of memory.
  return nullptr;
}

rx_byte* buddy_allocator::reallocate_unlocked(void* _data, rx_size _size) {
  if (RX_HINT_LIKELY(_data)) {
    const auto region = reinterpret_cast<block*>(_data) - 1;

    const auto head = reinterpret_cast<block*>(m_head);
    const auto tail = reinterpret_cast<block*>(m_tail);

    RX_ASSERT(region >= head, "out of heap");
    RX_ASSERT(region <= tail - 1, "out of heap");

    // No need to resize.
    if (region->size >= needed(_size)) {
      return reinterpret_cast<rx_byte*>(_data);
    }

    // Create a new allocation.
    auto resize = allocate_unlocked(_size);
    if (RX_HINT_LIKELY(resize)) {
      memcpy(resize, _data, region->size - sizeof *region);
      deallocate_unlocked(_data);
      return resize;
    }

    // Out of memory.
    return nullptr;
  }

  return allocate_unlocked(_size);
}

void buddy_allocator::deallocate_unlocked(void* _data) {
  if (RX_HINT_LIKELY(_data)) {
    const auto region = reinterpret_cast<block*>(_data) - 1;

    const auto head = reinterpret_cast<block*>(m_head);
    const auto tail = reinterpret_cast<block*>(m_tail);

    RX_ASSERT(region >= head, "out of heap");
    RX_ASSERT(region <= tail - 1, "out of heap");

    region->free = true;
  }
}

} // namespace rx::memory
