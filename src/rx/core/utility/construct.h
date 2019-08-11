#ifndef RX_CORE_UTILITY_CONSTRUCT_H
#define RX_CORE_UTILITY_CONSTRUCT_H
#include "rx/core/types.h" // rx_size
#include "rx/core/assert.h" // RX_ASSERT
#include "rx/core/utility/forward.h"

#include "rx/core/memory/allocator.h"

struct rx_placement_new {};

inline void* operator new(rx_size, void* _data, rx_placement_new) {
  return _data;
}

namespace rx::utility {

template<typename T, typename... Ts>
inline void construct(void *_data, Ts&&... _args) {
  new (reinterpret_cast<T*>(_data), rx_placement_new{})
    T(utility::forward<Ts>(_args)...);
}

template<typename T, typename... Ts>
inline T* allocate_and_construct(memory::allocator* _allocator, Ts&&... _args) {
  void* data{_allocator->allocate(sizeof(T))};
  if (data) {
    construct<T>(data, utility::forward<Ts>(_args)...);
    return reinterpret_cast<T*>(data);
  }
  return nullptr;
}

} // namespace rx::utility

#endif // RX_CORE_UTILITY_CONSTRUCT_H
