#include "rx/core/config.h"

#include "rx/core/memory/copy.h"
#include "rx/core/memory/fill.h"

#include "rx/core/math/pow.h"
#include "rx/core/math/ldexp.h"

#define STBI_NO_BMP
#define STBI_NO_PSD
#define STBI_NO_GIF
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STBI_NO_STDIO

#define STBI_memcpy(dst_, _src, _size)   Rx::Memory::copy_untyped((dst_), (_src), (_size))
#define STBI_memset(dst_, _value, _size) Rx::Memory::fill_untyped((dst_), (_value), (_size))

#define STBI_pow(_base, _exp)            Rx::Math::pow((_base), (_exp))
#define STBI_ldexp(_x, _exp)             Rx::Math::ldexp((_x), (_exp))

#define STBI_ASSERT(_x) RX_ASSERT((_x), "stb_image")

#if defined(RX_COMPILER_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#elif defined(RX_COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "lib/stb_image.h"

#if defined(RX_COMPILER_GCC)
#pragma GCC diagnostic pop
#elif defined(RX_COMPILER_CLANG)
#pragma clang diagnostic pop
#endif
