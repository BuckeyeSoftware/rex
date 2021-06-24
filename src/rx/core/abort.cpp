#include "rx/core/log.h"
#include "rx/core/abort.h"

#include "rx/core/concurrency/spin_lock.h"
#include "rx/core/concurrency/scope_unlock.h"

#include "rx/core/hints/unreachable.h"

#if defined(RX_PLATFORM_POSIX)
#include <signal.h> // raise, SIGABRT
#elif defined(RX_PLATFORM_WINDOWS)
#include <intrin.h> // __debugbreak
#include <stdlib.h> // exit
#endif

namespace Rx {

RX_LOG("abort", logger);

static constexpr const Size MAX_HANDLERS = 4;

struct AbortHandler {
  AbortHandlerFn function;
  void* user;
};

static Concurrency::SpinLock s_spin_lock;
static Concurrency::AtomicFlag s_calling_handlers;
static AbortHandler s_handlers[MAX_HANDLERS];
static Uint8 s_count;

bool register_abort_handler(AbortHandlerFn _function, void* _user) {
  Concurrency::ScopeLock lock{s_spin_lock};
  if (s_count < MAX_HANDLERS) {
    s_handlers[s_count++] = { _function, _user };
    return true;
  }
  return false;
}

[[noreturn, maybe_unused]]
static void abort_debug() {
#if defined(RX_PLATFORM_POSIX) && (defined(RX_COMPILER_GCC) || defined(RX_COMPILER_CLANG))
  __builtin_trap();
#elif defined(RX_PLATFORM_WINDOWS)
  __debugbreak();
#else
  static_assert(0, "implement abort_debug");
#endif
}

[[noreturn, maybe_unused]]
static void abort_release() {
#if defined(RX_PLATFORM_POSIX)
  raise(SIGABRT);
#elif defined(RX_PLATFORM_WINDOWS)
  // Windows doesn't support SIGABRT. If we use standard abort when built with
  // VS's debug runtime library, we'll get the annoying:
  //
  // "This application has requested the Runtime to terminate in an unusual way."
  //
  // Try to avoid this using exit instead.
  ::exit(2);
#else
  static_assert(0, "implement abort_release");
#endif
  RX_HINT_UNREACHABLE();
}

[[noreturn]]
void abort_message(const char* _message, bool _truncated) {
  logger->error(_truncated ? "%s... [truncated]" : "%s", _message);

  // Forcefully flush the current log contents before we abort, so that any
  // messages that may include the reason for the abortion end up in the log.
  Log::flush();

  // Prevent recursion when calling handlers.
  if (!s_calling_handlers.test_and_set()) {
    // Keep the lock unheld when calling the handler as it could add another.
    Concurrency::ScopeLock lock{s_spin_lock};
    for (auto handler = s_handlers; handler->function; handler++) {
      Concurrency::ScopeUnlock unlock{s_spin_lock};
      handler->function(_message, handler->user);
    }
  }

#if defined(RX_DEBUG)
  abort_debug();
#else
  abort_release();
#endif
}

} // namespace Rx
