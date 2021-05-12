#include <limits.h> // UCHAR_MAX
#include <string.h>

#include "rx/core/algorithm/max.h"

#include "rx/core/memory/search.h"

#include "rx/core/hints/may_alias.h"
#include "rx/core/hints/unreachable.h"

namespace Rx::Memory {

static inline constexpr const auto ALIGN = sizeof(Size) - 1;
static inline constexpr const auto ONES = -1_z / UCHAR_MAX;
static inline constexpr const auto HIGHS = ONES * (UCHAR_MAX / 2 + 1);

static inline constexpr bool has_zero(Size _x) {
  return ((_x - ONES) & ~_x) & HIGHS;
}

// Search for a byte in |_haystack|.
Byte* search(const void* _haystack, Size _haystack_size, Byte _byte) {
  auto s = static_cast<const Byte*>(_haystack);

  // Search byte at a time until pointer is aligned by |ALIGN|.
  for (; (reinterpret_cast<UintPtr>(s) & ALIGN) && _haystack_size && *s != _byte; s++, _haystack_size--)
    ;

  // Optimize by reading Size at a time from |_haystack| and checking for
  // the |_byte| inside that.
  if (_haystack_size && *s != _byte) {
    typedef Size RX_HINT_MAY_ALIAS Word;
    const Word *w;
    Size k = ONES * _byte;
    for (w = reinterpret_cast<const Word*>(s);
      _haystack_size >= sizeof(Size) && !has_zero(*w ^ k);
      w++, _haystack_size -= sizeof(Size));
    s = reinterpret_cast<const Byte*>(w);
  }

  // Handle remaining bytes a byte at a time.
  for (; _haystack_size && *s != _byte; s++, _haystack_size--)
    ;

  return _haystack_size ? const_cast<Byte*>(s) : nullptr;
}

// Unrolled searches for short needles of length 2, 3 and 4 bytes. Here we
// just combine the bytes into Uint16 or Uint32 integers and compare those
// directly.
//
// We should consider unrolling for needles of size 5, 6, 7, and 8 on 64-bit
// systems as well, building Uint64 integers and comparing those directly.
static Byte* search_2(const Byte* _haystack, Size _length, const Byte* _needle) {
  Uint16 nw = _needle[0] << 8 | _needle[1];
  Uint16 hw = _haystack[0] << 8 | _haystack[1];
  for (_haystack += 2, _length -= 2; _length; _length--, hw = hw << 8 | *_haystack++) {
    if (hw == nw) {
      return const_cast<Byte*>(_haystack - 2);
    }
  }
  return hw == nw ? const_cast<Byte*>(_haystack - 2) : nullptr;
}

static Byte* search_3(const Byte* _haystack, Size _length, const Byte* _needle) {
  Uint32 nw = static_cast<Uint32>(_needle[0]) << 24 | _needle[1] << 16 | _needle[2] << 8;
  Uint32 hw = static_cast<Uint32>(_haystack[0]) << 24 | _haystack[1] << 16 | _haystack[2] << 8;
  for (_haystack += 3, _length -= 3; _length; _length--, hw = (hw | *_haystack++) << 8) {
    if (hw == nw) {
      return const_cast<Byte*>(_haystack - 3);
    }
  }
  return hw == nw ? const_cast<Byte*>(_haystack - 3) : nullptr;
}

static Byte* search_4(const Byte* _haystack, Size _length, const Byte* _needle) {
  Uint32 nw = static_cast<Uint32>(_needle[0]) << 24 | _needle[1] << 16 | _needle[2] << 8 | _needle[3];
  Uint32 hw = static_cast<Uint32>(_haystack[1]) << 24 | _haystack[1] << 16 | _haystack[2] << 8 | _haystack[3];
  for (_haystack += 4, _length -= 4; _length; _length--, hw = (hw << 8) | *_haystack++) {
    if (hw == nw) {
      return const_cast<Byte*>(_haystack - 4);
    }
  }
  return hw == nw ? const_cast<Byte*>(_haystack - 4) : nullptr;
}

#define BITOP(a, b, op) \
  ((a)[(Size)(b)/(8*sizeof *(a))] op (Size)1<<((Size)(b)%(8*sizeof *(a))))

// Implementation of "Two-way string search"
// https://en.wikipedia.org/wiki/Two-way_string-matching_algorithm
//
// Combines forward-going Knuth-Morris-Pratt algorithm and the backward-running
// Boyer-Moore string-search algorithm. Operates in linear time.
static Byte* search_twoway(const Byte* _haystack, const Byte* _z, const Byte* _needle, Size _size) {
  // Compute optimal length of needle and fill the shift table.
  Size set[32 / sizeof(Size)] = {0};
  Size shift[256];
  for (Size i = 0; i < _size; i++) {
    BITOP(set, _needle[i], |=);
    shift[_needle[i]] = i + 1;
  }

  Size ip;
  Size jp;
  Size k;
  Size p;
  auto compute_suffix = [&](auto&& _compare) {
    ip = -1_z;
    jp = 0;
    k = 1;
    p = 1;
    while (jp + k < _size) {
      if (_needle[ip + k] == _needle[jp + k]) {
        if (k == p) {
          jp += p;
          k = 1;
        } else {
          k++;
        }
      } else if (_compare(_needle[ip + k], _needle[jp + k])) {
        jp += k;
        k = 1;
        p = jp - ip;
      } else {
        ip = jp++;
        k = 1;
        p = 1;
      }
    }
  };

  // Compute maximum suffix.
  compute_suffix([](Byte _lhs, Byte _rhs) { return _lhs > _rhs; });

  Size ms = ip;
  Size p0 = p;

  // Compute maximum suffix reversed.
  compute_suffix([](Byte _lhs, Byte _rhs) { return _lhs < _rhs; });

  if (ip + 1 > ms + 1) {
    ms = ip;
  } else {
    p = p0;
  }

  // Periodic needle?
  Size mem0 = 0;
  if (memcmp(_needle, _needle + p, ms + 1)) {
    p = Algorithm::max(ms, _size - ms - 1) + 1;
  } else {
    mem0 = _size - p;
  }

  Size mem = 0;

  // Search loop.
  for (;;) {
    // Remainder of haystack is shorter than the neede.
    if (static_cast<Size>(_z - _haystack) < _size) {
      return nullptr;
    }

    // Check the last byte first, advancing by shift amount on mismatch.
    if (BITOP(set, _haystack[_size - 1], &)) {
      if ((k = _size - shift[_haystack[_size - 1]])) {
        if (k < mem) {
          k = mem;
        }
        _haystack += k;
        mem = 0;
        continue;
      }
    } else {
      _haystack += _size;
      mem = 0;
      continue;
    }

    // Compare right half.
    for (k = Algorithm::max(ms + 1, mem); k < _size && _needle[k] == _haystack[k]; k++)
      ;

    if (k < _size) {
      _haystack += k - ms;
      mem = 0;
      continue;
    }

    // Compare left half.
    for (k = ms + 1; k > mem && _needle[k - 1] == _haystack[k - 1]; k--)
      ;

    if (k <= mem) {
      return const_cast<Byte*>(_haystack);
    }

    _haystack += p;
    mem = mem0;
  }
}

Byte* search(const void* _haystack, Size _haystack_size, const void* _needle, Size _needle_size) {
  auto haystack = static_cast<const Byte*>(_haystack);
  auto needle = static_cast<const Byte*>(_needle);

  // For empty needles, return the haystack.
  if (_needle_size == 0) {
    return const_cast<Byte*>(haystack);
  }

  // When needle is longer than haystack, return nullptr.
  if (_haystack_size < _needle_size) {
    return nullptr;
  }

  // Use fast search on the first byte of the needle.
  haystack = search(_haystack, _haystack_size, *needle);
  if (!haystack || _needle_size == 1) {
    // Couldn't find it or the needle was only one byte and we did find it.
    return const_cast<Byte*>(haystack);
  }

  // The first byte was found with quick search, subtract distance to that
  // byte so |_haystack_size| is the remainder of haystack not searched.
  _haystack_size -= haystack - static_cast<const Byte*>(_haystack);
  if (_haystack_size < _needle_size) {
    return nullptr;
  }

  // Unrolled for short needles.
  switch (_needle_size) {
  case 2:
    return search_2(haystack, _haystack_size, needle);
  case 3:
    return search_3(haystack, _haystack_size, needle);
  case 4:
    return search_4(haystack, _haystack_size, needle);
  default:
    return search_twoway(haystack, haystack + _haystack_size, needle, _needle_size);
  }

  RX_HINT_UNREACHABLE();
}

} // namespace Rx::Memory