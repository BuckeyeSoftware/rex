#include "rx/core/abort.h" // abort
#include "rx/core/assert.h"

namespace Rx {

[[noreturn]]
void assert_message(const char* _expression,
  const SourceLocation& _source_location, const char* _message, bool _truncated)
{
  abort("Assertion failed:\n"
        "  Expression:  %s\n"
        "  Location:    %s:%d\n"
        "  Function:    %s\n"
        "  Description: %s%s",
        _expression, _source_location.file(), _source_location.line(),
          _source_location.function(), _message, _truncated ? "... [truncated]" : "");
}

} // namespace Rx
