#ifndef RX_CORE_TAGGED_POINTER_H
#define RX_CORE_TAGGED_POINTER_H
#include "rx/core/memory/allocator.h" // alignment
#include "rx/core/assert.h"

namespace Rx {

/// Tagged pointer
///
/// As most allocations are aligned by \link Memory::ALIGNMENT there is often
/// several always-zero bits in a pointer which can be used to store short tag
/// values. This class enables such constructions and provides helper functions
/// to decode the pointer and tag and some additional assertions to detect
/// tag value overflow.
template<typename T>
struct TaggedPtr {
  TaggedPtr(T* _ptr = nullptr, Byte _tag = 0);

  /// Retag the pointer with a new tag value.
  void retag(Byte _tag);

  /// Decode the pointer.
  T* as_ptr() const;

  /// Decode the tag.
  Byte as_tag() const;

private:
  static inline constexpr auto TAG_MASK = Memory::Allocator::ALIGNMENT - 1;
  static inline constexpr auto PTR_MASK = ~TAG_MASK;

  union {
    T* m_as_ptr;
    UintPtr m_as_bits;
  };
};

template<typename T>
TaggedPtr<T>::TaggedPtr(T* _ptr, Byte _tag) {
  RX_ASSERT((reinterpret_cast<UintPtr>(_ptr) & TAG_MASK) == 0,
    "pointer not aligned");
  RX_ASSERT((_tag & PTR_MASK) == 0, "tag value too large");
  m_as_ptr = _ptr;
  m_as_bits |= _tag;
}

template<typename T>
void TaggedPtr<T>::retag(Byte _tag) {
  RX_ASSERT((_tag & PTR_MASK) == 0, "tag value too large");
  m_as_ptr = as_ptr();
  m_as_bits |= _tag;
}

template<typename T>
T* TaggedPtr<T>::as_ptr() const {
  return reinterpret_cast<T*>(m_as_bits & PTR_MASK);
}

template<typename T>
Byte TaggedPtr<T>::as_tag() const {
  return m_as_bits & TAG_MASK;
}

} // namespace Rx

#endif // RX_CORE_TAGGED_POINTER_H
