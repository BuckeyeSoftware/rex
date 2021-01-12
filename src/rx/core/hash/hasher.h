#ifndef RX_CORE_HASH_PRIMITIVE_H
#define RX_CORE_HASH_PRIMITIVE_H
#include "rx/core/hash/mix_pointer.h"
#include "rx/core/hash/mix_float.h"
#include "rx/core/hash/mix_enum.h"
#include "rx/core/hash/mix_int.h"

namespace Rx::Hash {

template<typename T>
concept Hashable = requires(const T& _value) {
  _value.hash();
};

template<typename T>
struct Hasher;

template<Hashable T>
struct Hasher<T> {
  Size operator()(const T& _value) const {
    return _value.hash();
  }
};

template<Concepts::Enum T>
struct Hasher<T> {
  Size operator()(const T& _enum) const {
    return mix_enum(_enum);
  }
};

template<>
struct Hasher<bool> {
  Size operator()(bool _value) const {
    return mix_bool(_value);
  }
};

template<>
struct Hasher<signed char> {
  Size operator()(char _value) const {
    return mix_sint8(_value);
  }
};

template<>
struct Hasher<signed short> {
  Size operator()(signed short _value) const {
    return mix_sint16(_value);
  }
};

template<>
struct Hasher<signed int> {
  Size operator()(signed int _value) const {
    return mix_sint32(_value);
  }
};

template<>
struct Hasher<signed long> {
  Size operator()(signed long _value) const {
    if constexpr (sizeof _value == 8) {
      return mix_sint64(_value);
    } else {
      return mix_sint32(_value);
    }
  }
};

template<>
struct Hasher<signed long long> {
  Size operator()(signed long long _value) const {
    return mix_sint64(_value);
  }
};

template<>
struct Hasher<unsigned char> {
  Size operator()(unsigned char _value) const {
    return mix_uint8(_value);
  }
};

template<>
struct Hasher<unsigned short> {
  Size operator()(unsigned short _value) const {
    return mix_uint16(_value);
  }
};

template<>
struct Hasher<unsigned int> {
  Size operator()(unsigned int _value) const {
    return mix_uint32(_value);
  }
};

template<>
struct Hasher<unsigned long> {
  Size operator()(unsigned long _value) const {
    if constexpr (sizeof _value == 8) {
      return mix_uint32(_value);
    } else {
      return mix_uint32(_value);
    }
  }
};

template<>
struct Hasher<unsigned long long> {
  Size operator()(unsigned long long _value) const {
    return mix_uint64(_value);
  }
};

template<>
struct Hasher<Float32> {
  Size operator()(Float32 _value) const {
    return mix_float32(_value);
  }
};

template<>
struct Hasher<Float64> {
  Size operator()(Float64 _value) const {
    return mix_float64(_value);
  }
};

template<typename T>
struct Hasher<T*> {
  Size operator()(const T* _value) const {
    return mix_pointer(_value);
  }
};

} // namespace Rx

#endif // RX_CORE_HASH_H
