#ifndef RX_CORE_REPORT_H
#define RX_CORE_REPORT_H
#include "rx/core/string.h"
#include "rx/core/log.h"

namespace Rx {

struct Log;

struct Report {
  struct Error {
    constexpr Error() = default;
    operator bool() const { return false; }
    operator decltype(nullopt)() const { return nullopt; }
    operator decltype(nullptr)() const { return nullptr; }
    template<typename T>
    operator Optional<T>() const { return nullopt; }
  };

  Report(Memory::Allocator& _allocator, Log& _log);
  Report(Report&& report_);
  Report& operator=(Report&& report_);

  template<typename... Ts>
  RX_HINT_FORMAT(3, 0)
  [[nodiscard]] bool log(Log::Level _level, const char* _format, Ts&&... _arguments) const;

  template<typename... Ts>
  RX_HINT_FORMAT(2, 0)
  Error error(const char* _format, Ts&&... _arguments) const;

  // Rename this report.
  bool rename(const String& _name);

  const String& name() const;

private:
  [[nodiscard]] bool write(Log::Level _level, const char* _data) const;

  String m_name;
  Log* m_log;
};

inline Report::Report(Memory::Allocator& _allocator, Log& _log)
  : m_name{_allocator}
  , m_log{&_log}
{
}

inline Report::Report(Report&& report_)
  : m_name{Utility::move(report_.m_name)}
  , m_log{Utility::exchange(report_.m_log, nullptr)}
{
}

inline Report& Report::operator=(Report&& report_) {
  if (this != &report_) {
    m_name = Utility::move(report_.m_name);
    m_log = Utility::exchange(report_.m_log, nullptr);
  }
  return *this;
}

template<typename... Ts>
bool Report::log(Log::Level _level, const char* _format, Ts&&... _arguments) const {
  // Don't format the contents unless there are format arguments.
  if constexpr (sizeof...(Ts) != 0) {
    const auto format = String::format(m_name.allocator(), _format,
      Utility::forward<Ts>(_arguments)...);
    return write(_level, format.data());
  } else {
    return write(_level, _format);
  }
}

template<typename... Ts>
Report::Error Report::error(const char* _format, Ts&&... _arguments) const {
  RX_ASSERT(log(Log::Level::ERROR, _format, Utility::forward<Ts>(_arguments)...),
    "report log failure");
  return {};
}

inline const String& Report::name() const {
  return m_name;
}

} // namespace Rx

#endif // RX_CORE_REPORT_H