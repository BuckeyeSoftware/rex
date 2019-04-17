#ifndef RX_CORE_UTILITY_DESTRUCT_H
#define RX_CORE_UTILITY_DESTRUCT_H

#include <rx/core/memory/allocator.h>

namespace rx::utility {

template<typename T>
inline void destruct(void* _data) {
  reinterpret_cast<T*>(_data)->~T();
}

template<typename T, typename... Ts>
inline void destruct_and_deallocate(memory::allocator* _allocator, void* _data) {
  if (_data) {
    destruct<T>(_data);
    _allocator->deallocate(reinterpret_cast<rx_byte*>(_data));
  }
}

} // namespace rx::utility

#endif // RX_CORE_UTILITY_DESTRUCT_H
