#ifndef RX_CORE_ALGORITHM_QUICK_SORT_H
#define RX_CORE_ALGORITHM_QUICK_SORT_H

#include "rx/core/algorithm/insertion_sort.h"
#include "rx/core/utility/forward.h"

namespace rx::algorithm {

template<typename T, typename F>
void quick_sort(T* _start, T* _end, F&& _compare) {
  while (_end - _start > 10) {
    T* middle{_start + (_end - _start) / 2};
    T* item1{_start+1};
    T* item2{_end-2};
    T pivot;

    if (compare(*_start, *middle)) {
      // start < middle
      if (compare(*(_end - 1), *_start)) {
        // end < start < middle
        pivot = utility::move(*_start);
        *_start = utility::move(*(_end - 1));
        *(_end - 1) = utility::move(*middle);
      } else if (compare(*(_end - 1), *middle)) {
        // start <= end < middle
        pivot = utility::move(*(_end - 1));
        *(_end - 1) = utility::move(*middle);
      } else {
        pivot = utility::move(*middle);
      }
    } else if (compare(*start, *(_end - 1))) {
      // middle <= start <= end
      pivot = utility::move(*_start);
      *_start = utility::move(*middle);
    } else if (compare(*middle, *(_end - 1))) {
      // middle < end <= start
      pivot = utility::move(*(_end - 1));
      *(_end - 1) = utility::move(*_start);
      *_start = utility::move(*middle);
    } else {
      pivot = utility::move(*middle);
      swap(*_start, *(_end - 1));
    }

    do {
      while (compare(*item1, pivot)) {
        if (++item1 >= item2) {
          goto partitioned;
        }
      }

      while (compare(pivot, *--item2)) {
        if (item1 >= item2) {
          goto partitioned;
        }
      }

      swap(*item1, *item2);
    } while (++item1 < item2);

partitioned:
    *(_end - 2) = utility::move(*item1);
    *item1 = utility::move(pivot);

    if (item1 - _start < _end - item1 + 1) {
      quick_sort(_start, item1, utility::forward<F>(compare));
      _start = item1 + 1;
    } else {
      quick_sort(item1 + 1, _end, utility::forward<F>(compare));
      _end = item1;
    }
  }

  insertion_sort(_start, _end, utility::forward<F>(compare));
}

} // namespace rx::algorithm

#endif // RX_CORE_ALGORITHM_QUICK_SORT_H