#ifndef RX_CORE_LOG_H
#define RX_CORE_LOH_H

#include <rx/core/statics.h> // static_global
#include <rx/core/string.h> // string
#include <rx/core/pp.h> // RX_PP_STRINGIZE

namespace rx {

struct log {
  enum class level {
    warning,
    info,
    verbose,
    error
  };

  constexpr log(const char* name, const char* file_name, int line);

  template<typename... Ts>
  void operator()(const char* fmt, level lvl, Ts&&... args);

  const char* name() const;

private:
  void write(string&& contents, level lvl);

  const char* m_name;
  const char* m_file_name;
  int m_line;
};

inline constexpr log::log(const char* name, const char* file_name, int line)
  : m_name{name}
  , m_file_name{file_name}
  , m_line{line}
{
}

template<typename... Ts>
inline void log::operator()(const char* fmt, log::level lvl, Ts&&... args) {
  write({fmt, forward<Ts>(args)...}, lvl);
}

inline const char* log::name() const {
  return m_name;
}

#define RX_LOG(name) \
  static ::rx::static_global<::rx::log> name \
    ("log_" RX_PP_STRINGIZE(name), RX_PP_STRINGIZE(name), __FILE__, __LINE__)

} // namespace rx

#endif // RX_CORE_LOG_H
