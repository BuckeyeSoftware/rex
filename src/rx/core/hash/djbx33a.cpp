#include "rx/core/hash/djbx33a.h"
#include "rx/core/hash/combine.h"

#include "rx/core/hints/assume_aligned.h"

#include "rx/core/utility/wire.h"

#if defined(__SSE2__)
#include <emmintrin.h>
#endif

namespace Rx::Hash {

Array<Byte[16]> djbx33a(const Byte* _data, Size _size) {
  alignas(16) Uint32 state_vector[4] = { 5381, 5381, 5381, 5381 };

#if defined(__SSE2__)
  Size lane = 0;

  // Align to 16 bytes.
  for (; _size && reinterpret_cast<UintPtr>(_data) & 15; _size--, _data++) {
    const auto value = state_vector[lane];
    state_vector[lane] = (value << 5) + value + *_data;
    lane = (lane + 1) & 3;
  }

  // Rotate.
  auto p = reinterpret_cast<__m128i*>(state_vector);
  for (Size i = 0; i < lane; i++) {
    const auto v = state_vector[0];
    state_vector[0] = state_vector[1];
    state_vector[1] = state_vector[2];
    state_vector[2] = state_vector[3];
    state_vector[3] = v;
  }

  // Process 16 bytes at a time.
  RX_HINT_ASSUME_ALIGNED(_data, 16);
  const __m128i zero = _mm_setzero_si128();
  __m128i state = _mm_load_si128(reinterpret_cast<const __m128i*>(state_vector));
  __m128i word0, word1;
  for (; _size >= 16; _size -= 16, _data += 16) {
    __m128i i = _mm_load_si128(reinterpret_cast<const __m128i*>(_data));

    // qword0: low
    word0 = _mm_unpacklo_epi8(i, zero);

    word1 = _mm_add_epi32(_mm_unpacklo_epi8(word0, zero), state);
    state = _mm_add_epi32(_mm_slli_epi32(state, 5), word1);

    // dword1
    word1 = _mm_add_epi32(_mm_unpackhi_epi8(word0, zero), state);
    state = _mm_add_epi32(_mm_slli_epi32(state, 5), word1);

    // qword1: high
    word0 = _mm_unpackhi_epi8(i, zero);

    word1 = _mm_add_epi32(_mm_unpacklo_epi8(word0, zero), state);
    state = _mm_add_epi32(_mm_slli_epi32(state, 5), word1);

    // dword3
    word1 = _mm_add_epi32( _mm_unpackhi_epi8(word0, zero), state);
    state = _mm_add_epi32(_mm_slli_epi32(state, 5), word1);
  }

  _mm_store_si128(p, state);

  // Rotate back, and store.
  for (Size i = 0; i < lane; i++) {
    auto v = state_vector[3];
    state_vector[3] = state_vector[2];
    state_vector[2] = state_vector[1];
    state_vector[1] = state_vector[0];
    state_vector[0] = v;
  }

  // Handle remainder.
  for (; _size; _size--, _data++) {
    const auto value = state_vector[lane];
    state_vector[lane] = (value << 5) + value + *_data;
    lane = (lane + 1) & 3;
  }
#else
  Uint32 s = 0;
  for (auto end = _data + _size, p = _data; p < end; p++) {
    state_vector[s] = (state_vector[s] << 5) + state_vector[s] + *p;
    s = (s + 1) & 3;
  }
#endif

  const auto& words = state_vector;

  Array<Byte[16]> result;
  Utility::write_u32(result.data() + 0, words[0]);
  Utility::write_u32(result.data() + 4, words[1]);
  Utility::write_u32(result.data() + 8, words[2]);
  Utility::write_u32(result.data() + 12, words[3]);
  return result;
}

} // namespace Rx::Hash
