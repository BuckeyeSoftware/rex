#include "rx/core/memory/system_allocator.h"
#include "rx/core/memory/move.h"

#define STBIW_MALLOC(_size)             Rx::Memory::SystemAllocator::instance().allocate((_size))
#define STBIW_REALLOC(_ptr, _size)      Rx::Memory::SystemAllocator::instance().reallocate((_ptr), (_size))
#define STBIW_FREE(_ptr)                Rx::Memory::SystemAllocator::instance().deallocate((_ptr))

#define STBI_MEMMOVE(dst_, _src, _size) Rx::Memory::move_untyped((dst_), (_src), (_size))

#define STBIW_ASSERT(_expression)       RX_ASSERT((_expression), "stb_image_write")

#if defined(RX_COMPILER_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#elif defined(RX_COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "lib/stb_image_write.h"

#if defined(RX_COMPILER_GCC)
#pragma GCC diagnostic pop
#elif defined(RX_COMPILER_CLANG)
#pragma clang diagnostic pop
#endif