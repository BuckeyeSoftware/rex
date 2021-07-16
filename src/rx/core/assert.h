#ifndef RX_CORE_ASSERT_H
#define RX_CORE_ASSERT_H
#include "rx/core/format.h"
#include "rx/core/source_location.h"

/// \file assert.h
namespace Rx {

/// \brief Log an assertion message.
/// \param _expression The asserting expression.
/// \param _source_location The SourceLocation of the assertion.
/// \param _message The assertion message.
/// \param _truncate If the message was truncated.
/// \warning This function does not return.
[[noreturn]] RX_API void assert_message(const char* _expression,
  const SourceLocation& _source_location, const char* _message, bool _truncated);

/// \brief Log a formatted assertion message.
/// \param _expression The asserting expression.
/// \param _source_location The SourceLocation of the assertion.
/// \param _format The format string.
/// \param _arguments The format arguments.
/// \warning This function does not return.
template<typename... Ts>
[[noreturn]] RX_HINT_FORMAT(3, 0)
void assert_fail(const char* _expression, const SourceLocation& _source_location,
  const char* _format, Ts&&... _arguments)
{
  // When we have format arguments use an on-stack format buffer.
  if constexpr(sizeof...(Ts) > 0) {
    char buffer[4096];
    const Size length = format_buffer({buffer, sizeof buffer}, _format,
      Utility::forward<Ts>(_arguments)...);
    assert_message(_expression, _source_location, buffer, length >= sizeof buffer);
  } else {
    assert_message(_expression, _source_location, _format, false);
  }
}

} // namespace Rx

#if defined(RX_DOCUMENT)
/// \brief Assert condition
/// \param _expression Expression to assert.
/// \param ... Format string and arguments for the assertion.
/// \note The expression given will always be executed regardless if assertions
/// are enabled or disabled. Only the output of assertion failures will be
/// disabled in release builds.
#define RX_ASSERT(_expression, ...)
#else // defined(RX_DOCUMENT)
#if defined(RX_DEBUG)
#define RX_ASSERT(_condition, ...) \
  (static_cast<void>((_condition) \
    || (::Rx::assert_fail(#_condition, RX_SOURCE_LOCATION, __VA_ARGS__), 0)))
#else // defined(RX_DEBUG)
#define RX_ASSERT(_condition, ...) \
  static_cast<void>((_condition) || 0)
#endif // defined(RX_DEBUG)
#endif // defined(RX_DOCUMENT)

#endif // RX_CORE_ASSERT_H
