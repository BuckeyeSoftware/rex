#ifndef RX_CORE_LOG_H
#define RX_CORE_LOG_H
#include "rx/core/event.h"
#include "rx/core/string.h"
#include "rx/core/source_location.h"

namespace rx {

struct stream;

struct log {
  enum class level {
    k_warning,
    k_info,
    k_verbose,
    k_error
  };

  using queue_event = event<void(level, string)>;
  using write_event = event<void(level, string)>;
  using flush_event = event<void()>;

  constexpr log(const char* _name, const source_location& location);

  [[nodiscard]] static bool subscribe(stream* _stream);
  [[nodiscard]] static bool unsubscribe(stream* _stream);
  [[nodiscard]] static bool enqueue(log* _owner, level _level, string&& _contents);

  static void flush();

  // Write a formatted message given by |_format| and |_arguments| of associated
  // severity level |_level|. This will queue the given message on the logger
  // thread.
  //
  // All delegates given by |on_queue| will be called immediately by this
  // function (and thus on the same thread).
  //
  // This function is thread-safe.
  template<typename... Ts>
  bool write(level _level, const char* _format, Ts&&... _arguments);

  // Convenience functions that call |write| with the appropriate severity
  // level, given by their name.
  //
  // All of these functions are thread-safe.
  template<typename... Ts>
  bool warning(const char* _format, Ts&&... _arguments);
  template<typename... Ts>
  bool info(const char* _format, Ts&&... _arguments);
  template<typename... Ts>
  bool verbose(const char* _format, Ts&&... _arguments);
  template<typename... Ts>
  bool error(const char* _format, Ts&&... _arguments);

  // When a message is queued, all delegates associated by this function are
  // called. This is different from |on_write| in that |callback_| is called
  // by the same thread which calls |write|, |warning|, |info|, |verbose|, or
  // |error| immediately.
  //
  // This function returns the event handle, keep the handle alive for as
  // long as you want the delegate |callback_| to be called for such event.
  //
  // This function is thread-safe.
  queue_event::handle on_queue(queue_event::delegate&& callback_);

  // When a message is written, all delegates associated by this function are
  // called. This is different from |on_queue| in that |callback_| is called
  // by the logging thread when the actual message is written.
  //
  // This function returns the event handle, keep the handle alive for as
  // long as you want the delegate |callback_| to be called for such event.
  //
  // This function is thread-safe.
  write_event::handle on_write(write_event::delegate&& callback_);

  // When all messages queued for this log are actually written, all delegates
  // associated by this function are called.
  //
  // This function returns an event handle, keep the handle alive for as long
  // as you want the delegate |callback_| to be called for such event.
  //
  // This function is thread-safe.
  flush_event::handle on_flush(flush_event::delegate&& callback_);

  // Query the name of the logger, which is the name given to the |RX_LOG| macro.
  const char* name() const;

  // Query the source information of where this log is defined.
  const source_location& source_info() const &;

  void signal_write(level _level, string&& contents_);
  void signal_flush();

private:
  const char* m_name;
  source_location m_source_location;

  queue_event m_queue_event;
  write_event m_write_event;
  flush_event m_flush_event;
};

inline constexpr log::log(const char* _name, const source_location& _source_location)
  : m_name{_name}
  , m_source_location{_source_location}
{
}

template<typename... Ts>
inline bool log::write(level _level, const char* _format, Ts&&... _arguments) {
  if (sizeof...(Ts) != 0) {
    auto format = string::format(_format, utility::forward<Ts>(_arguments)...);
    m_queue_event.signal(_level, format);
    return enqueue(this, _level, utility::move(format));
  } else {
    m_queue_event.signal(_level, _format);
    return enqueue(this, _level, {_format});
  }
}

template<typename... Ts>
inline bool log::warning(const char* _format, Ts&&... _arguments) {
  return write(level::k_warning, _format, utility::forward<Ts>(_arguments)...);
}

template<typename... Ts>
inline bool log::info(const char* _format, Ts&&... _arguments) {
  return write(level::k_info, _format, utility::forward<Ts>(_arguments)...);
}

template<typename... Ts>
inline bool log::verbose(const char* _format, Ts&&... _arguments) {
  return write(level::k_verbose, _format, utility::forward<Ts>(_arguments)...);
}

template<typename... Ts>
inline bool log::error(const char* _format, Ts&&... _arguments) {
  return write(level::k_error, _format, utility::forward<Ts>(_arguments)...);
}

inline const char* log::name() const {
  return m_name;
}

inline const source_location& log::source_info() const & {
  return m_source_location;
}

inline log::queue_event::handle log::on_queue(queue_event::delegate&& callback_) {
  return m_queue_event.connect(utility::move(callback_));
}

inline log::write_event::handle log::on_write(write_event::delegate&& callback_) {
  return m_write_event.connect(utility::move(callback_));
}

inline log::flush_event::handle log::on_flush(flush_event::delegate&& callback_) {
  return m_flush_event.connect(utility::move(callback_));
}

#define RX_LOG(_name, _identifier) \
  static ::rx::global<::rx::log> _identifier{"loggers", (_name), (_name), \
    ::rx::source_location{__FILE__, "(global constructor)", __LINE__}}

} // namespace rx

#endif // RX_CORE_LOG_H
