#include "rx/core/config.h"

#define STB_IMAGE_IMPLEMENTATION

#define STBI_NO_BMP
#define STBI_NO_PSD
#define STBI_NO_GIF
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STBI_NO_STDIO

#if defined(RX_COMPILER_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter" // unused parameter
#endif

#include "lib/stb_image.h"

#if defined(RX_COMPILER_GCC)
#pragma GCC diagnostic pop
#endif