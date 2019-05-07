#include <stdlib.h> // abort

#include <rx/core/abort.h>
#include <rx/core/statics.h>
#include <rx/core/log.h>

RX_LOG("abort", abort_log);

namespace rx {

[[noreturn]]
void abort(const char* _message) {
  // deinitialize all static globals that were not manually initialized
  static_globals::fini();

  abort_log(log::level::k_error, "%s", _message);

  // deinitialize static globals that were manually initialized
  static_globals::find("log")->fini();

  static_globals::find("system_allocator")->fini();
  static_globals::find("malloc_allocator")->fini();

  // use system abort
  ::abort();
}

} // namespace rx