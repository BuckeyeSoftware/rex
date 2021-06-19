#ifndef RX_CORE_ALGORITHM_QUICK_SORT_H
#define RX_CORE_ALGORITHM_QUICK_SORT_H
#include "rx/core/algorithm/insertion_sort.h"
#include "rx/core/utility/swap.h"

/// \file quick_sort.h
namespace Rx::Algorithm {

/// \brief Quick sort
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
void quick_sort(T* start_, T* end_, F&& _compare) {
  while (end_ - start_ > 10) {
    T* middle = start_ + (end_ - start_) / 2;
    T* item1 = start_ + 1;
    T* item2 = end_ - 2;
    T pivot;

    if (_compare(*start_, *middle)) {
      // start < middle
      if (_compare(*(end_ - 1), *start_)) {
        // end < start < middle
        pivot = Utility::move(*start_);
        *start_ = Utility::move(*(end_ - 1));
        *(end_ - 1) = Utility::move(*middle);
      } else if (_compare(*(end_ - 1), *middle)) {
        // start <= end < middle
        pivot = Utility::move(*(end_ - 1));
        *(end_ - 1) = Utility::move(*middle);
      } else {
        pivot = Utility::move(*middle);
      }
    } else if (_compare(*start_, *(end_ - 1))) {
      // middle <= start <= end
      pivot = Utility::move(*start_);
      *start_ = Utility::move(*middle);
    } else if (_compare(*middle, *(end_ - 1))) {
      // middle < end <= start
      pivot = Utility::move(*(end_ - 1));
      *(end_ - 1) = Utility::move(*start_);
      *start_ = Utility::move(*middle);
    } else {
      pivot = Utility::move(*middle);
      Utility::swap(*start_, *(end_ - 1));
    }

    do {
      while (_compare(*item1, pivot)) {
        if (++item1 >= item2) {
          goto partitioned;
        }
      }

      while (_compare(pivot, *--item2)) {
        if (item1 >= item2) {
          goto partitioned;
        }
      }

      Utility::swap(*item1, *item2);
    } while (++item1 < item2);

partitioned:
    *(end_ - 2) = Utility::move(*item1);
    *item1 = Utility::move(pivot);

    if (item1 - start_ < end_ - item1 + 1) {
      quick_sort(start_, item1, Utility::forward<F>(_compare));
      start_ = item1 + 1;
    } else {
      quick_sort(item1 + 1, end_, Utility::forward<F>(_compare));
      end_ = item1;
    }
  }

  insertion_sort(start_, end_, Utility::forward<F>(_compare));
}

} // namespace Rx::Algorithm

#endif // RX_CORE_ALGORITHM_QUICK_SORT_H
