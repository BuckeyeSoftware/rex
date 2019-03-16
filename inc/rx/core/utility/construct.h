#ifndef RX_CORE_UTILITY_CONSTRUCT_H
#define RX_CORE_UTILITY_CONSTRUCT_H

#include <rx/core/types.h> // rx_size
#include <rx/core/assert.h> // RX_ASSERT
#include <rx/core/utility/forward.h>

struct rx_placement_new {};

inline void* operator new(rx_size, void* _data, rx_placement_new) {
  return _data;
}
namespace rx::utility {

template<typename T, typename... Ts>
inline void construct(void *_data, Ts&&... _args) {
  new (reinterpret_cast<T*>(_data), rx_placement_new{}) T(utility::forward<Ts>(_args)...);
}

} // namespace rx::utility

#endif // RX_CORE_UTILITY_CONSTRUCT_H
