#ifndef RX_CORE_ALGORITHM_H
#define RX_CORE_ALGORITHM_H

#include <rx/core/utility/move.h>
#include <rx/core/utility/forward.h>

namespace rx {

template<typename T>
inline void swap(T &_lhs, T &_rhs) {
  T tmp{utility::move(_lhs)};
  _lhs = utility::move(_rhs);
  _rhs = utility::move(tmp);
}

template<typename T>
inline T min(T _value) {
  return _value;
}

template<typename T, typename... Ts>
inline T min(T _a, T _b, Ts&&... _args) {
  return min(_a < _b ? _a : _b, utility::forward<Ts>(_args)...);
}

template<typename T>
inline T max(T _a) {
  return _a;
}

template<typename T, typename... Ts>
inline T max(T _a, T _b, Ts&&... _args) {
  return max(_a > _b ? _a : _b, utility::forward<Ts>(_args)...);
}

template<typename T, typename F>
inline void insertion_sort(T* start, T* end, F&& compare) {
  for (T* item1 = start + 1; item1 < end; item1++) {
    if (compare(*item1, *(item1 - 1))) {
      T temp{utility::move(*item1)};
      *item1 = utility::move(*(item1 - 1));
      T* item2 = item1 - 1;
      for (; item2 > start && compare(temp, *(item2 - 1)); --item2) {
        *item2 = utility::move(*(item2 - 1));
      }
      *item2 = utility::move(temp);
    }
  }
}

template<typename T, typename F>
void sort(T* start, T* end, F&& compare) {
  while (end - start > 10) {
    T* middle{start + (end - start) / 2};
    T* item1{start+1};
    T* item2{end-2};
    T pivot;

    if (compare(*start, *middle)) {
      // start < middle
      if (compare(*(end - 1), *start)) {
        // end < start < middle
        pivot = utility::move(*start);
        *start = utility::move(*(end - 1));
        *(end - 1) = utility::move(*middle);
      } else if (compare(*(end - 1), *middle)) {
        // start <= end < middle
        pivot = utility::move(*(end - 1));
        *(end - 1) = utility::move(*middle);
      } else {
        pivot = utility::move(*middle);
      }
    } else if (compare(*start, *(end - 1))) {
      // middle <= start <= end
      pivot = utility::move(*start);
      *start = utility::move(*middle);
    } else if (compare(*middle, *(end - 1))) {
      // middle < end <= start
      pivot = utility::move(*(end - 1));
      *(end - 1) = utility::move(*start);
      *start = utility::move(*middle);
    } else {
      pivot = utility::move(*middle);
      swap(*start, *(end - 1));
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
    *(end - 2) = utility::move(*item1);
    *item1 = utility::move(pivot);

    if (item1 - start < end - item1 + 1) {
      sort(start, item1, utility::forward<F>(compare));
      start = item1 + 1;
    } else {
      sort(item1 + 1, end, utility::forward<F>(compare));
      end = item1;
    }
  }

  insertion_sort(start, end, utility::forward<F>(compare));
}

} // namespace rx

#endif // RX_CORE_ALGORITHM_H
