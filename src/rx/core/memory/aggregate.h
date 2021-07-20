#ifndef RX_CORE_MEMORY_AGGREGATE_H
#define RX_CORE_MEMORY_AGGREGATE_H
#include "rx/core/vector.h"

/// \file aggregate.h

namespace Rx::Memory {

/// \brief An aggregate allocator.
///
/// A way to define an aggregate allocation from multiple separate, differently
/// sized and aligned allocations of objects (or arrays of objects) as one
/// contiguous allocation with appropriate offsets to derive pointers that
/// maintain alignment and prevent overlap.
struct RX_API Aggregate {
  constexpr Aggregate(Allocator& _allocator);

  /// The offset of an item.
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

  /// \brief Allocate the aggregate.
  Byte* allocate();

private:
  struct Entry {
    Size size;
    Size align;
    Size offset;
  };

  Allocator& m_allocator;
  Vector<Entry> m_entries;
};

inline constexpr Aggregate::Aggregate(Allocator& _allocator)
  : m_allocator{_allocator}
  , m_entries{m_allocator}
{
}

inline Size Aggregate::operator[](Size _index) const {
  return m_entries[_index].offset;
}

template<typename T>
bool Aggregate::add(Size _count) {
  return add(sizeof(T), alignof(T), _count);
}

} // namespace Rx::Memory

#endif // RX_CORE_MEMORY_AGGREGATE_H
