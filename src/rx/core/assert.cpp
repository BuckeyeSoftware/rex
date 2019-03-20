#include <stdlib.h> // abort
#include <stdarg.h> // va_{list,start,end,copy}
#include <stdio.h> // vsnprintf

#include <rx/core/log.h> // RX_LOG, rx::log, rx::static_globals

RX_LOG("assert", assert_print);

namespace rx {

[[noreturn]]
void assert_fail(const char* expression, const char* file,
  const char* function, int line, const char* message, ...)
{
  va_list va;
  va_start(va, message);

  // calculate length to format
  va_list ap;
  va_copy(ap, va);
  const int length{vsnprintf(nullptr, 0, message, ap)};
  va_end(ap);

  // format into string
  string contents;
  contents.resize(length);
  vsnprintf(contents.data(), contents.size() + 1, message, va);

  va_end(va);
  assert_print(log::level::k_error, "Assertion failed: %s (%s:%d %s) \"%s\"",
    expression, file, line, function, utility::move(contents));

  // deinitialize all static globals
  static_globals::fini();

  static_globals::find("log")->fini();

  static_globals::find("system_allocator")->fini();
  static_globals::find("malloc_allocator")->fini();

  abort();
}

} // namespace rx
