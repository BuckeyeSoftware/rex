#include "rx/core/memory/fill.h"
#include "rx/core/memory/copy.h"
#include "rx/core/memory/system_allocator.h"

#include "rx/core/math/sqrt.h"
#include "rx/core/math/mod.h"
#include "rx/core/math/sin.h"
#include "rx/core/math/cos.h"
#include "rx/core/math/tan.h"
#include "rx/core/math/ceil.h"
#include "rx/core/math/floor.h"

#define nvg__memcpy(dst_, _src, _size)   Rx::Memory::copy_untyped((dst_), (_src), (_size))
#define nvg__memset(dst_, _value, _size) Rx::Memory::fill_untyped((dst_), (_value), (_size))

#define nvg__malloc(_size)               Rx::Memory::SystemAllocator::instance().allocate((_size))
#define nvg__realloc(_ptr, _size)        Rx::Memory::SystemAllocator::instance().reallocate((_ptr), (_size))
#define nvg__free(_ptr)                  Rx::Memory::SystemAllocator::instance().deallocate((_ptr))

#define nvg__sqrtf(_x)                   Rx::Math::sqrt((_x))
#define nvg__modf(_x, _y)                Rx::Math::mod((_x), (_y))
#define nvg__sinf(_x)                    Rx::Math::sin((_x))
#define nvg__cosf(_x)                    Rx::Math::cos((_x))
#define nvg__acosf(_x)                   Rx::Math::acos((_x))
#define nvg__tanf(_x)                    Rx::Math::tan((_x))
#define nvg__atan2f(_x, _y)              Rx::Math::atan2((_x), (_y))
#define nvg__ceilf(_x)                   Rx::Math::ceil((_x))

#define NANOVG_IMPLEMENTATION
#include "lib/nanovg.h"
