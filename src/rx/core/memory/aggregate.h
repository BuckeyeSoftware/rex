#ifndef RX_CORE_MEMORY_AGGREGATE_H
#define RX_CORE_MEMORY_AGGREGATE_H
#include "rx/core/assert.h"

/// \file aggregate.h

namespace Rx::Memory {

/// \brief A memory aggregate.
///
/// A way to define an aggregate allocation from multiple separate, differently
/// sized and aligned allocations of objects (or arrays of objects) as one
/// contiguous allocation with appropriate offsets to derive pointers that
/// maintain alignment and prevent overlap.
struct RX_API Aggregate {
  constexpr Aggregate();

  /// The number of bytes needed for this aggregate.
  /// \note Asserts if finalize() hasn't been called.
  Size bytes() const;

  /// The offset of an item.
  /// \note Asserts if finalize() hasn't been called.
  Size operator[](Size _index) const;

  /// \brief Add an object of type T to the aggregate.
  /// \param _count The number of objects of type T.
  /// \return On success, \c true. Otherwise, \c false.
  template<typename T>
  bool add(Size _count);

  /// \brief Add an object of specific size and alignment.
  /// \param _size The size of the object in bytes.
  /// \param _alignment The alignment of the object in bytes.
  /// \param _count The number of objects.
  /// \return On success, \c true. Otherwise, \c false.
  bool add(Size _size, Size _alignment, Size _count);

  /// \brief Finalize the aggregate for use.
  /// \return On success, \c true. Otherwise, \c false.
  bool finalize();

private:
  struct Entry {
    Size size;
    Size align;
    Size offset;
  };

  union {
    struct {} m_nat;
    Entry m_entries[64];
  };

  Size m_size;
  Size m_bytes;
};

inline constexpr Aggregate::Aggregate()
  : m_nat{}
  , m_size{0}
  , m_bytes{0}
{
}

inline Size Aggregate::bytes() const {
  RX_ASSERT(m_bytes, "not finalized");
  return m_bytes;
}

inline Size Aggregate::operator[](Size _index) const {
  RX_ASSERT(m_bytes, "not finalized");
  return m_entries[_index].offset;
}

template<typename T>
bool Aggregate::add(Size _count) {
  return add(sizeof(T), alignof(T), _count);
}

} // namespace Rx::Memory

#endif // RX_CORE_MEMORY_AGGREGATE_H
