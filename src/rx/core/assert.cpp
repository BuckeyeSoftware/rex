#include <stdio.h> // fprintf, stderr, fflush
#include <stdlib.h> // abort

#include <rx/core/assert.h>

[[noreturn]]
void rx::assert_fail(const char* expression, const char* message,
  const char* file, const char* function, int line)
{
  fprintf(stderr, "Assertion failed: %s \"%s\" (%s: %s: %d)\n",
    expression, message, file, function, line);
  fflush(nullptr);
  abort();
}
