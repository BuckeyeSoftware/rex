#include <string.h> // memcpy, memcmp

#include "rx/core/memory/debug_allocator.h"
#include "rx/core/concurrency/scope_lock.h"
#include "rx/core/assert.h"

namespace rx::memory {

// Random bits to OR into the pre and post canary values so that exact pointer
// values are not being stored. These need to be less than |k_alignment| so
// that we're ORing into known 0-bits.
//
// We should pick values that are not multiples of two so it's immediately
// obvious these are not actually real pointers.
static constexpr const auto k_pre_canary_bits = 0x7;
static constexpr const auto k_post_canary_bits = 0x3;

// Helper functions to calculate canary values to use before and after each
// allocated region.
static inline rx_uintptr pre_canary(rx_byte* _data) {
  return reinterpret_cast<rx_uintptr>(_data) | k_pre_canary_bits;
}

static inline rx_uintptr post_canary(rx_byte* _data) {
  return reinterpret_cast<rx_uintptr>(_data) | k_post_canary_bits;
}

struct alignas(allocator::k_alignment) metadata {
  rx_uintptr canary;
  rx_size size;
};

// Adjust a size that is about to be passed to allocation functions to make
// room for data used for debug instrumentation.
static inline rx_size adjust_size(rx_size _size) {
  return _size + sizeof(metadata) + sizeof(rx_uintptr);
}

// Wraps a just alllocated region with our canary values.
static rx_byte* box(rx_byte* _data, rx_size _size) {
  if (!_data) {
    return nullptr;
  }

  auto* pre = reinterpret_cast<metadata*>(_data);
  _data += sizeof *pre;
  auto* post = reinterpret_cast<metadata*>(_data + _size);

  // Write the leading canary value.
  pre->canary = pre_canary(_data);
  pre->size = _size;

  // Use memcpy for the trailing canary to avoid unaligned writes.
  rx_uintptr canary = post_canary(_data);
  memcpy(&post->canary, &canary, sizeof canary);

  return _data;
}

// Unwrap a canary into the original alllocated pointer.
static rx_byte* unbox(rx_byte* _data, const char* _caller) {
  if (!_data) {
    return nullptr;
  }

  auto* pre = reinterpret_cast<metadata*>(_data - sizeof(metadata));
  auto* post = reinterpret_cast<metadata*>(_data + pre->size);

  // Check the leading canary (buffer underflow).
  RX_ASSERT(pre->canary == pre_canary(_data),
    "Buffer underflow in heap memory pointed to by %p in %s", _data, _caller);

  // Check the trailing canary (buffer overflow).
  const auto canary = post_canary(_data);
  RX_ASSERT(memcmp(&post->canary, &canary, sizeof canary) == 0,
    "Buffer overflow in heap memory pointed to by %p in %s", _data, _caller);

  return reinterpret_cast<rx_byte*>(pre);
}

rx_byte* debug_allocator::track(rx_byte* _data) {
  concurrency::scope_lock lock{m_lock};
  for (rx_size i = 0; i < k_max_tracking; i++) {
    if (m_tracked[i] == 0) {
      m_tracked[i] = reinterpret_cast<rx_uintptr>(_data);
      return _data;
    }
  }

  RX_ASSERT(false, "Too many active allocations for debug allocator to track");
  return _data;
}

void debug_allocator::untrack(rx_byte* _data, const char* _caller) {
  if (!_data) {
    return;
  }

  concurrency::scope_lock lock{m_lock};
  const auto pointer = reinterpret_cast<rx_uintptr>(_data);
  for (rx_size i = 0; i < k_max_tracking; i++) {
    if (m_tracked[i] == pointer) {
      m_tracked[i] = 0;
      return;
    }
  }

  RX_ASSERT(false, "Attempt to %s pointer %p that was never allocated", _caller, _data);
}

rx_byte* debug_allocator::allocate(rx_size _size) {
  auto size = adjust_size(_size);
  auto data = m_allocator.allocate(size);
  data = box(data, _size);
  track(data);
  return data;
}

rx_byte* debug_allocator::reallocate(void* _data, rx_size _size) {
  auto data = reinterpret_cast<rx_byte*>(_data);
  auto size = adjust_size(_size);
  untrack(data, "reallocate");
  data = unbox(data, "reallocate");
  data = m_allocator.reallocate(data, size);
  data = box(data, _size);
  track(data);
  return data;
}

void debug_allocator::deallocate(void* _data) {
  if (!_data) {
    return;
  }

  auto data = reinterpret_cast<rx_byte*>(_data);
  untrack(data, "deallocate");

  // Write garbage all over the region to try and expose use-after-free bugs.
  auto* pre = reinterpret_cast<metadata*>(data - sizeof(metadata));
  for (rx_size i = 0; i < pre->size; i++) {
    data[i] ^= static_cast<rx_byte>(~i);
  }

  data = unbox(data, "deallocate");
  m_allocator.deallocate(data);
}

} // namespace rx::memory
