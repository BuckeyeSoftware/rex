#include "rx/core/config.h"

#include "rx/core/math/pow.h"
#include "rx/core/math/ldexp.h"

#define STB_IMAGE_IMPLEMENTATION

#define STBI_NO_BMP
#define STBI_NO_PSD
#define STBI_NO_GIF
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STBI_NO_STDIO

#define STBI_ASSERT(x) ((void)(x))

#define pow(x, y) Rx::Math::pow((x), (y))
#define ldexp(x, n) Rx::Math::ldexp((x), (n))

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
