#ifndef RX_CORE_TAGGED_POINTER_H
#define RX_CORE_TAGGED_POINTER_H
#include "rx/core/memory/allocator.h" // alignment

namespace rx {

// # Tagged pointer
//
// Since all allocations are aligned by |k_alignment|. There's bits that are
// always zero for heap allocated memory. We can use those bits to store a
// tag value.
//
// This class enables such constructions and provides helper functions to
// decode the pointer and tag.
template<typename T>
struct tagged_ptr {
  tagged_ptr(T* _ptr = nullptr, rx_byte _tag = 0);

  void retag(rx_byte _tag);

  T* ptr() const;
  rx_byte tag() const;

private:
  static inline constexpr auto k_tag_mask = memory::allocator::k_alignment - 1;
  static inline constexpr auto k_ptr_mask = ~k_tag_mask;

  union {
    T* m_as_ptr;
    rx_uintptr m_as_bits;
  };
};

template<typename T>
inline tagged_ptr<T>::tagged_ptr(T* _ptr, rx_byte _tag) {
  RX_ASSERT((reinterpret_cast<rx_uintptr>(_ptr) & k_tag_mask) == 0, "pointer not aligned");
  RX_ASSERT((_tag & k_ptr_mask) == 0, "tag value too large");
  m_as_ptr = _ptr;
  m_as_bits |= _tag;
}

template<typename T>
inline void tagged_ptr<T>::retag(rx_byte _tag) {
  RX_ASSERT((_tag & k_ptr_mask) == 0, "tag value too large");
  m_as_ptr = ptr();
  m_as_bits |= _tag;
}

template<typename T>
inline T* tagged_ptr<T>::ptr() const {
  return reinterpret_cast<T*>(m_as_bits & k_ptr_mask);
}

template<typename T>
inline rx_byte tagged_ptr<T>::tag() const {
  return m_as_bits & k_tag_mask;
}

} // namespace rx

#endif // RX_CORE_TAGGED_POINTER_H
