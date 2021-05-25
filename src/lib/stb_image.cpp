#include <string.h> // strcpy, strncpy
#include <stdlib.h> // strtol

#include "rx/core/config.h"

#include "rx/core/math/pow.h"
#include "rx/core/math/ldexp.h"

#include "rx/core/memory/copy.h"
#include "rx/core/memory/fill.h"

#define STB_IMAGE_IMPLEMENTATION

#define STBI_NO_BMP
#define STBI_NO_PSD
#define STBI_NO_GIF
#define STBI_NO_PIC
#define STBI_NO_PNM
// #define STBI_NO_STDIO

#define STBI_ASSERT(x) ((void)(x))

#define STBI_pow(x, y) Rx::Math::pow((x), (y))
#define STBI_ldexp(x, n) Rx::Math::ldexp((x), (n))

#define STBI_memcpy Rx::Memory::copy_untyped
#define STBI_memset Rx::Memory::fill_untyped

#define STBI_strcpy strcpy
#define STBI_strcmp strcmp
#define STBI_strncmp strncmp
#define STBI_strtol strtol

#if defined(RX_COMPILER_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#elif defined(RX_COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#endif

#include "lib/stb_image.h"

#if defined(RX_COMPILER_GCC)
#pragma GCC diagnostic pop
#elif defined(RX_COMPILER_CLANG)
#pragma clang diagnostic pop
#endif
