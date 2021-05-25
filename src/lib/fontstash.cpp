#include "rx/core/memory/fill.h"
#include "rx/core/memory/copy.h"

#include "rx/core/math/floor.h"

#include "rx/core/memory/system_allocator.h"

#define fons__memset(...)  Rx::Memory::fill_untyped(__VA_ARGS__)
#define fons__memcpy(...)  Rx::Memory::copy_untyped(__VA_ARGS__)

#define fons__malloc(...)  Rx::Memory::SystemAllocator::instance().allocate(__VA_ARGS__)
#define fons__realloc(...) Rx::Memory::SystemAllocator::instance().reallocate(__VA_ARGS__)
#define fons__free(...)    Rx::Memory::SystemAllocator::instance().deallocate(__VA_ARGS__)

#define fons__floorf(...)  Rx::Math::floor(__VA_ARGS__)

#define FONTSTASH_IMPLEMENTATION
#include "fontstash.h"