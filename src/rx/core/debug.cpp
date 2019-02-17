#include <stdarg.h> // va_{list, start, end}
#include <stdio.h> // vfprintf

#include <rx/core/debug.h>

void rx::debug_message(const char *file, const char *function, int line,
  const char *message, ...)
{
  fprintf(stderr, "debug: ");
  va_list va;
  va_start(va, message);
  vfprintf(stderr, message, va);
  va_end(va);
}
