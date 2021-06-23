#ifndef RX_CORE_SOURCE_LOCATION_H
#define RX_CORE_SOURCE_LOCATION_H
#include "rx/core/config.h" // RX_COMPILER_{GCC,CLANG}

namespace Rx {

#if defined(RX_COMPILER_GCC) || defined(RX_COMPILER_CLANG)
#define RX_FUNCTION __PRETTY_FUNCTION__
#else // defined(RX_COMPILER_GCC) || defined(RX_COMPILER_CLANG)
#define RX_FUNCTION __func__
#endif // defined(RX_COMPILER_GCC) || defined(RX_COMPILER_CLANG)

// Don't emit file or function strings in anything but debug builds.
#if defined(RX_DEBUG)
#define RX_SOURCE_LOCATION \
  ::Rx::SourceLocation{__FILE__, __func__, RX_FUNCTION, __LINE__}
#else
#define RX_SOURCE_LOCATION \
  ::Rx::SourceLocation{"(unknown)", "(unknown)", "(unknown)", 0}
#endif // defined(RX_DEBUG)

struct SourceLocation {
  constexpr SourceLocation(const char* _file,
    const char* _function, const char* _pretty_function, int _line);

  const char* file() const;
  const char* function() const;
  const char* pretty_function() const;
  int line() const;

private:
  const char* m_file;
  const char* m_function;
  const char* m_pretty_function;
  int m_line;
};

inline constexpr SourceLocation::SourceLocation(const char* _file,
  const char* _function, const char* _pretty_function, int _line)
  : m_file{_file}
  , m_function{_function}
  , m_pretty_function{_pretty_function}
  , m_line{_line}
{
}

inline const char* SourceLocation::file() const {
  return m_file;
}

inline const char* SourceLocation::function() const {
  return m_function;
}

inline const char* SourceLocation::pretty_function() const {
  return m_pretty_function;
}

inline int SourceLocation::line() const {
  return m_line;
}

} // namespace Rx

#endif // RX_CORE_SOURCE_LOCATION_H
