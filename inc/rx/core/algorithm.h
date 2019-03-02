#ifndef RX_CORE_ALGORITHM_H
#define RX_CORE_ALGORITHM_H

#include <rx/core/traits.h> // move, forward

namespace rx {

template<typename T>
inline void swap(T &lhs, T &rhs) {
  T tmp{move(lhs)};
  lhs = move(rhs);
  rhs = move(tmp);
}

template<typename T>
inline T min(T a) {
  return a;
}

template<typename T, typename... Ts>
inline T min(T a, T b, Ts&&... args) {
  return min(a < b ? a : b, forward<Ts>(args)...);
}

template<typename T>
inline T max(T a) {
  return a;
}

template<typename T, typename... Ts>
inline T max(T a, T b, Ts&&... args) {
  return max(a > b ? a : b, forward<Ts>(args)...);
}

template<typename T, typename F>
inline void insertion_sort(T* start, T* end, F&& compare) {
  for (T* item1 = start + 1; item1 < end; item1++) {
    if (compare(*item1, *(item1 - 1))) {
      T temp{move(*item1)};
      *item1 = move(*(item1 - 1));
      T* item2 = item1 - 1;
      for (; item2 > start && compare(temp, *(item2 - 1)); --item2) {
        *item2 = move(*(item2 - 1));
      }
      *item2 = move(temp);
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
        pivot = move(*start);
        *start = move(*(end - 1));
        *(end - 1) = move(*middle);
      } else if (compare(*(end - 1), *middle)) {
        // start <= end < middle
        pivot = move(*(end - 1));
        *(end - 1) = move(*middle);
      } else {
        pivot = move(*middle);
      }
    } else if (compare(*start, *(end - 1))) {
      // middle <= start <= end
      pivot = move(*start);
      *start = move(*middle);
    } else if (compare(*middle, *(end - 1))) {
      // middle < end <= start
      pivot = move(*(end - 1));
      *(end - 1) = move(*start);
      *start = move(*middle);
    } else {
      pivot = move(*middle);
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
    *(end - 2) = move(*item1);
    *item1 = move(pivot);

    if (item1 - start < end - item1 + 1) {
      sort(start, item1, forward<F>(compare));
      start = item1 + 1;
    } else {
      sort(item1 + 1, end, forward<F>(compare));
      end = item1;
    }
  }

  insertion_sort(start, end, forward<F>(compare));
}

} // namespace rx

#endif // RX_CORE_ALGORITHM_H
