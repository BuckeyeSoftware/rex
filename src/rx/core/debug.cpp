#include <stdarg.h> // va_{list, start, end}
#include <stdio.h> // vfprintf

#include <rx/core/debug.h>

#include <rx/core/concurrency/spin_lock.h>
#include <rx/core/concurrency/scope_lock.h>

using rx::concurrency::scope_lock;
using rx::concurrency::spin_lock;

static spin_lock g_lock;

void rx::debug_message(const char *file, const char *function, int line,
  const char *message, ...)
{
  scope_lock<spin_lock> locked(g_lock);

  fprintf(stderr, "debug: ");
  va_list va;
  va_start(va, message);
  vfprintf(stderr, message, va);
  va_end(va);
}
