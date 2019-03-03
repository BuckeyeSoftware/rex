#include <stdlib.h> // abort
#include <stdarg.h> // va_{list,start,end,copy}
#include <stdio.h> // vsnprintf

#include <rx/core/log.h> // RX_LOG, rx::log, rx::static_globals

RX_LOG("assert", assert_print);

[[noreturn]]
void rx::assert_fail(const char* expression, const char* file,
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
  rx::string contents;
  contents.resize(length);
  vsnprintf(contents.data(), contents.size() + 1, message, va);

  va_end(va);
  assert_print(rx::log::level::k_error, "Assertion failed: %s (%s:%d %s) \"%s\"",
    expression, file, line, function, rx::move(contents));

  // deinitialize all static globals
  rx::static_globals::fini();

  // these were explicitly enabled with ->init in main so fini above does
  // not finalize them, finalize them here
  rx::static_globals::find("system_allocator")->fini();
  rx::static_globals::find("log")->fini();

  abort();
}
