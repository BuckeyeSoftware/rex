#include <stdlib.h> // abort

#include <rx/core/types.h>
#include <rx/core/assert.h>

void* operator new(rx_size) {
  RX_ASSERT(false, "operator new is disabled");
  abort();
}

void* operator new[](rx_size) {
  RX_ASSERT(false, "operator new[] is disabled");
  abort();
}

void operator delete(void*) {
  RX_ASSERT(false, "operator delete is disabled");
  abort();
}

void operator delete(void*, rx_size) {
  RX_ASSERT(false, "operator delete is disabled");
  abort();
}

void operator delete[](void*) {
  RX_ASSERT(false, "operator delete[] is disabled");
  abort();
}

void operator delete[](void*, rx_size) {
  RX_ASSERT(false, "operator delete[] is disabled");
}

extern "C" {
  void __cxa_pure_virtual() {
    RX_ASSERT(false, "pure virtual function call");
  }

  void __cxa_guard_acquire(...) {
    // we don't require thread-safe initialization of statics
  }

  void __cxa_guard_release(...) {
    // we don't require thread-safe initialization of statics
  }
}