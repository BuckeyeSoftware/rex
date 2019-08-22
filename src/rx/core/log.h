#ifndef RX_CORE_LOG_H
#define RX_CORE_LOG_H
#include "rx/core/statics.h" // static_global
#include "rx/core/string.h" // string
#include "rx/core/pp.h" // RX_PP_STRINGIZE

namespace rx {

struct log {
  enum class level {
    k_warning,
    k_info,
    k_verbose,
    k_error
  };

  constexpr log(const char* name, const char* file_name, int line);

  template<typename... Ts>
  void operator()(level lvl, const char* fmt, Ts&&... args);

  const char* name() const;
  const char* file_name() const;
  int line() const;

private:
  void write(level lvl, string&& contents);

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
inline void log::operator()(log::level _level, const char* _format, Ts&&... _arguments) {
  write(_level, string::format(_format, utility::forward<Ts>(_arguments)...));
}

inline const char* log::name() const {
  return m_name;
}

inline const char* log::file_name() const {
  return m_file_name;
}

inline int log::line() const {
  return m_line;
}

#define RX_LOG(_name, _identifier) \
  static RX_GLOBAL<::rx::log> _identifier{"log_" _name, (_name), __FILE__, __LINE__}

} // namespace rx

#endif // RX_CORE_LOG_H
