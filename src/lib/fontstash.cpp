#include "rx/core/memory/copy.h"
#include "rx/core/memory/fill.h"
#include "rx/core/memory/system_allocator.h"

#include "rx/core/math/floor.h"

#define fons__memcpy(dst_, _src, _size)   Rx::Memory::copy_untyped((dst_), (_src), (_size))
#define fons__memset(dst_, _value, _size) Rx::Memory::fill_untyped((dst_), (_value), (_size))

#define fons__malloc(_size)               Rx::Memory::SystemAllocator::instance().allocate((_size))
#define fons__realloc(_ptr, _size)        Rx::Memory::SystemAllocator::instance().reallocate((_ptr), (_size))
#define fons__free(_ptr)                  Rx::Memory::SystemAllocator::instance().deallocate((_ptr))

#define fons__floorf(_x)                  Rx::Math::floor((_x))

#define FONTSTASH_IMPLEMENTATION
#include "lib/fontstash.h"