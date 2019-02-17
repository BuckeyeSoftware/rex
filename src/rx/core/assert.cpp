#include <stdio.h> // fprintf, stderr, fflush
#include <stdlib.h> // abort
#include <stdarg.h> // va_{list, start, end}

#include <rx/core/assert.h>

[[noreturn]]
void rx::assert_fail(const char* expression, const char* file,
  const char* function, int line, const char* message, ...)
{
  fprintf(stderr, "Assertion failed: %s (%s: %s: %d) \"",
    expression, file, function, line);

  va_list va;
  va_start(va, message);
  vfprintf(stderr, message, va);
  va_end(va);

  fprintf(stderr, "\"\n");

  fflush(nullptr);
  abort();
}
