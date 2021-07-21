#include "rx/core/types.h"
#include "rx/core/assert.h"
#include "rx/core/abort.h"

// These cannot be disabled in debug builds under Emscripten.
#if !defined(RX_PLATFORM_EMSCRIPTEN) || (defined(RX_PLATFORM_EMSCRIPTEN) && !defined(RX_DEBUG))
// Has to be in the std namespace since some compilers generate references to
// the mangled name.
namespace std {
  using size_t = Rx::Size;
  enum class align_val_t : size_t {};
  struct nothrow_t {
    explicit nothrow_t() = default;
  };
}

// Bring them into global namespace to have "std::"-free stringizatze for the
// messages passed to Rx::abort.
using std::size_t;
using std::align_val_t;
using std::nothrow_t;

// Templates for stamping out operator new and delete functions.
#define NEW_TEMPLATE(...) \
  void* operator __VA_ARGS__ { \
    Rx::abort("operator " #__VA_ARGS__ " is disabled"); \
  }

#define DELETE_TEMPLATE(...) \
  void operator __VA_ARGS__ noexcept { \
    Rx::abort("operator " #__VA_ARGS__ "is disabled"); \
  }

// Generate new and delete operators for objects and arrays respectively
// based on the varadic macro argument list.
#define NEW(...) \
  NEW_TEMPLATE(new(__VA_ARGS__)) \
  NEW_TEMPLATE(new[](__VA_ARGS__))

#define DELETE(...) \
  DELETE_TEMPLATE(delete(__VA_ARGS__)) \
  DELETE_TEMPLATE(delete[](__VA_ARGS__))

// Replaceable allocation functions.
NEW(size_t)
NEW(size_t, align_val_t)

// Replacable non-throwing allocation functions.
NEW(size_t, const nothrow_t&)
NEW(size_t, align_val_t, const nothrow_t&)

// Replacable usual deallocation functions.
DELETE(void*)
DELETE(void*, align_val_t)
DELETE(void*, size_t)
DELETE(void*, size_t, align_val_t)
#endif

extern "C" {
  void __cxa_pure_virtual() {
    Rx::abort("pure virtual function call");
  }

  // Don't define |__cxa_guard_{acquire, release}| when built with thread
  // sanitizer since it provides it's own versions of these runtime functions.
#if !defined(RX_TSAN)
  static constexpr Rx::Uint8 COMPLETE = 1 << 0;
  static constexpr Rx::Uint8 PENDING = 1 << 1;

  bool __cxa_guard_acquire(Rx::Uint8* guard_) {
    if (guard_[1] == COMPLETE) {
      return false;
    }

    if (guard_[1] & PENDING) {
      Rx::abort("recursive initialization unsupported");
    }

    guard_[1] = PENDING;
    return true;
  }

  void __cxa_guard_release(Rx::Uint8* guard_) {
    guard_[1] = COMPLETE;
  }
#endif
}
