#include <stdlib.h> // abort
#include <stdarg.h> // va_{list,start,end,copy}
#include <stdio.h> // vsnprintf

#include "rx/core/log.h" // RX_LOG, rx::log
#include "rx/core/abort.h" // rx::abort

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

  abort(contents.data());
}

} // namespace rx
