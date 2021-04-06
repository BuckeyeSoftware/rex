#ifndef RX_CORE_ALGORITHM_INSERTION_SORT_H
#define RX_CORE_ALGORITHM_INSERTION_SORT_H
#include "rx/core/utility/move.h"

/// \file insertion_sort.h

namespace Rx::Algorithm {

/// Insertion sort
/// \param start_ RandomAccessIterator to start of sequence to sort
/// \param end_ RandomAccessIterator to end of sequence to sort
/// \param _compare Invocable comparator functor taking two values of template parameter type T.
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
