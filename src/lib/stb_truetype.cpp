#include "rx/core/math/floor.h"
#include "rx/core/math/ceil.h"
#include "rx/core/math/sqrt.h"
#include "rx/core/math/pow.h"
#include "rx/core/math/mod.h"
#include "rx/core/math/cos.h"
#include "rx/core/math/abs.h"

#include "rx/core/memory/copy.h"
#include "rx/core/memory/fill.h"

#define STBTT_ifloor(_x)                  Rx::Sint32(Rx::Math::floor(_x))
#define STBTT_iceil(_x)                   Rx::Sint32(Rx::Math::ceil(_x))

#define STBTT_sqrt(_x)                    Rx::Math::sqrt((_x))
#define STBTT_pow(_x, _y)                 Rx::Math::pow((_x), (_y))
#define STBTT_fmod(_x, _y)                Rx::Math::mod((_x), (_y))
#define STBTT_cos(_x)                     Rx::Math::cos((_x))
#define STBTT_acos(_x)                    Rx::Math::acos((_x))
#define STBTT_fabs(_x)                    Rx::Math::abs((_x))
#define STBTT_memcpy(_dst, _src, _size)   Rx::Memory::copy_untyped((_dst), (_src), (_size))
#define STBTT_memset(_dst, _value, _size) Rx::Memory::fill_untyped((_dst), (_value), (_size))

#define STBTT_assert(_x) RX_ASSERT((_x), "stb_truetype")

#if defined(RX_COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable: 4244) // 'argument' conversion from 'double' to 'float', possible loss of data
#endif // defined(RX_COMPILER_MSVC)

#define STB_TRUETYPE_IMPLEMENTATION
#include "lib/stb_truetype.h"

#if defined(RX_COMPILER_MSVC)
#pragma warning(pop)
#endif // defined(RX_COMPILER_MSVC)
