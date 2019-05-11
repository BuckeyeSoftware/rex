#include <rx/core/types.h>
#include <rx/core/assert.h>
#include <rx/core/abort.h>

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

extern "C" {
  void __cxa_pure_virtual() {
    rx::abort("pure virtual function call");
  }

  void __cxa_guard_acquire(...) {
    // we don't require thread-safe initialization of statics
  }

  void __cxa_guard_release(...) {
    // we don't require thread-safe initialization of statics
  }
}