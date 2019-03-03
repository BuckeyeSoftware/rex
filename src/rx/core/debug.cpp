#include <stdlib.h> // abort
#include <stdarg.h> // va_{list,start,end,copy}
#include <stdio.h> // vsnprintf

#include <rx/core/log.h> // RX_LOG, rx::log
#include <rx/core/debug.h> // rx::debug_message

RX_LOG("debug", debug_print);

void rx::debug_message(const char* file, const char* function, int line,
  const char* message, ...)
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
  debug_print(rx::log::level::k_info, "%s:%d %s: \"%s\"", file, line, function,
    rx::move(contents));
}
