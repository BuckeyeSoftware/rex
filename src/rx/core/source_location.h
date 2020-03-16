#ifndef RX_CORE_SOURCE_LOCATION_H
#define RX_CORE_SOURCE_LOCATION_H
#include "rx/core/config.h"

namespace rx {

struct source_location {
  constexpr source_location(const char* _file,
    const char* _function, int _line);

  const char* file() const;
  const char* function() const;
  int line() const;

private:
  const char* m_file;
  const char* m_function;
  int m_line;
};

inline constexpr source_location::source_location(const char* _file,
  const char* _function, int _line)
  : m_file{_file}
  , m_function{_function}
  , m_line{_line}
{
}

inline const char* source_location::file() const {
  return m_file;
}

inline const char* source_location::function() const {
  return m_function;
}

inline int source_location::line() const {
  return m_line;
}

} // namespace rx

#endif // RX_CORE_SOURCE_LOCATION_H
