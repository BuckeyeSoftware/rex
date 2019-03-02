#ifndef RX_CORE_LOG_H
#define RX_CORE_LOH_H

#include <rx/core/statics.h> // static_global
#include <rx/core/string.h> // string
#include <rx/core/pp.h> // RX_PP_STRINGIZE

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
inline void log::operator()(log::level lvl, const char* fmt, Ts&&... args) {
  write(lvl, {fmt, forward<Ts>(args)...});
}

inline const char* log::name() const {
  return m_name;
}

#define RX_LOG(name) \
  static ::rx::static_global<::rx::log> name \
    ("log_" RX_PP_STRINGIZE(name), RX_PP_STRINGIZE(name), __FILE__, __LINE__)

} // namespace rx

#endif // RX_CORE_LOG_H
