#include "rx/core/memory/fill.h"
#include "rx/core/memory/copy.h"

#include "rx/core/math/sqrt.h"
#include "rx/core/math/mod.h"
#include "rx/core/math/sin.h"
#include "rx/core/math/cos.h"
#include "rx/core/math/tan.h"
#include "rx/core/math/ceil.h"
#include "rx/core/math/floor.h"

#include "rx/core/memory/system_allocator.h"

#define nvg__memset(...)   Rx::Memory::fill_untyped(__VA_ARGS__)
#define nvg__memcpy(...)   Rx::Memory::copy_untyped(__VA_ARGS__)

#define nvg__malloc(...)   Rx::Memory::SystemAllocator::instance().allocate(__VA_ARGS__)
#define nvg__realloc(...)  Rx::Memory::SystemAllocator::instance().reallocate(__VA_ARGS__)
#define nvg__free(...)     Rx::Memory::SystemAllocator::instance().deallocate(__VA_ARGS__)

#define nvg__sqrtf(...)    Rx::Math::sqrt(__VA_ARGS__)
#define nvg__modf(...)     Rx::Math::mod(__VA_ARGS__)
#define nvg__sinf(...)     Rx::Math::sin(__VA_ARGS__)
#define nvg__cosf(...)     Rx::Math::cos(__VA_ARGS__)
#define nvg__tanf(...)     Rx::Math::tan(__VA_ARGS__)
#define nvg__atan2f(...)   Rx::Math::atan2(__VA_ARGS__)
#define nvg__acosf(...)    Rx::Math::acos(__VA_ARGS__)
#define nvg__ceilf(...)    Rx::Math::ceil(__VA_ARGS__)

#define NANOVG_IMPLEMENTATION
#include "nanovg.h"
