#ifndef RX_CORE_FORMAT_H
#define RX_CORE_FORMAT_H

#include <rx/core/types.h> // rx_size

namespace rx {

template<typename T>
struct format {
  T operator()(const T& value) const {
    return value;
  }
};

template<rx_size E>
struct format<char[E]> {
  const char* operator()(const char (&data)[E]) const {
    return data;
  }
};

} // namespace rx

#endif // RX_CORE_FORMAT_H
