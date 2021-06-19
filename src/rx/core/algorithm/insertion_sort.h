#ifndef RX_CORE_ALGORITHM_INSERTION_SORT_H
#define RX_CORE_ALGORITHM_INSERTION_SORT_H
#include "rx/core/utility/move.h"

/// \file insertion_sort.h

namespace Rx::Algorithm {

/// \brief Insertion sort
///
/// Sorts elements of the array pointed by [ \p start_ and \p end_ ) using the
/// \p _compare invocable to determine the order.
///
/// The sorting algorithm used by this function compares pairs of elements by
/// invoking the specified \p _compare invocable with references to them as
/// argument.
///
/// The function does not return any value, but modifies the content of the
/// array pointed by [ \p start_, \p end_ ) by reordering its elements as
/// defined by \p _compare.
///
/// \param start_ Pointer to first object of the array to be sorted.
/// \param end_ Pointer to one past the last object of the array to be sorted.
/// \param _compare Invocable that compares two elements. This invocable is
/// called repeatedly to compare two elements. It shall follow the prototype:
///
///   bool compare(const T& lhs, const T& rhs);
///
/// \note This function has O(n log n) complexity
template<typename T, typename F>
void insertion_sort(T* start_, T* end_, F&& _compare) {
  for (T* item1 = start_ + 1; item1 < end_; item1++) {
    if (_compare(*item1, *(item1 - 1))) {
      T temp = Utility::move(*item1);
      *item1 = Utility::move(*(item1 - 1));
      T* item2 = item1 - 1;
      for (; item2 > start_ && _compare(temp, *(item2 - 1)); --item2) {
        *item2 = Utility::move(*(item2 - 1));
      }
      *item2 = Utility::move(temp);
    }
  }
}

} // namespace Rx::Algorithm

#endif // RX_CORE_ALGORITHM_INSERTION_SORT_H
