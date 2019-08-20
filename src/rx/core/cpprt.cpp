#include "rx/core/types.h"
#include "rx/core/assert.h"
#include "rx/core/abort.h"
#include "rx/core/hint.h"

#include "rx/core/concurrency/atomic.h"
#include "rx/core/concurrency/yield.h"

void* operator new(rx_size) {
  rx::abort("operator new is disabled");
}

void* operator new[](rx_size) {
  rx::abort("operator new[] is disabled");
}

void operator delete(void*) {
  rx::abort("operator delete is disabled");
}

void operator delete(void*, rx_size) {
  rx::abort("operator delete is disabled");
}

void operator delete[](void*) {
  rx::abort("operator delete[] is disabled");
}

void operator delete[](void*, rx_size) {
  rx::abort("operator delete[] is disabled");
}