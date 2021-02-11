#ifndef RX_CORE_LOG_H
#define RX_CORE_LOG_H
#include "rx/core/event.h"
#include "rx/core/string.h"
#include "rx/core/source_location.h"

namespace Rx {

struct Stream;

struct RX_API Log {
  RX_MARK_NO_COPY(Log);
  RX_MARK_NO_MOVE(Log);

  enum class Level : Uint8 {
    WARNING,
    INFO,
    VERBOSE,
    ERROR
  };

  using QueueEvent = Event<void(Level, String)>;
  using WriteEvent = Event<void(Level, String)>;
  using FlushEvent = Event<void()>;

  constexpr Log(const char* _name, const SourceLocation& _source_location);

  [[nodiscard]] static bool subscribe(Stream* _stream);
  [[nodiscard]] static bool unsubscribe(Stream* _stream);
  [[nodiscard]] static bool enqueue(Log* _owner, Level _level, String&& _contents);

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
  RX_HINT_FORMAT(3, 0)
  bool write(Level _level, const char* _format, Ts&&... _arguments);

  // Write a message given by |message_|.
  bool write(Level _level, String&& message_);

  // Convenience functions that call |write| with the appropriate severity
  // level, given by their name.
  //
  // All of these functions are thread-safe.
  template<typename... Ts>
  RX_HINT_FORMAT(2, 0)
  bool warning(const char* _format, Ts&&... _arguments);

  template<typename... Ts>
  RX_HINT_FORMAT(2, 0)
  bool info(const char* _format, Ts&&... _arguments);

  template<typename... Ts>
  RX_HINT_FORMAT(2, 0)
  bool verbose(const char* _format, Ts&&... _arguments);

  template<typename... Ts>
  RX_HINT_FORMAT(2, 0)
  bool error(const char* _format, Ts&&... _arguments);

  // When a message is queued, all delegates associated by this function are
  // called. This is different from |on_write| in that |callback_| is called
  // by the same thread which calls |write|, |warning|, |info|, |verbose|, or
  // |error| immediately.
  //
  // This function returns the event handle, or nullopt on error. Keep the
  // handle alive for as ong as you want the delegate |callback_| to be called
  // for such event.
  //
  // This function is thread-safe.
  template<typename F>
  Optional<QueueEvent::Handle> on_queue(F&& callback_);

  // When a message is written, all delegates associated by this function are
  // called. This is different from |on_queue| in that |callback_| is called
  // by the logging thread when the actual message is written.
  //
  // This function returns the event handle, or nullopt on error. Keep the
  // handle alive for as long as you want the delegate |callback_| to be called
  // for such event.
  //
  // This function is thread-safe.
  template<typename F>
  Optional<WriteEvent::Handle> on_write(F&& callback_);

  // When all messages queued for this log are actually written, all delegates
  // associated by this function are called.
  //
  // This function returns an event handle, or nullopt on error. Keep the handle
  // alive for as long as you want the delegate |callback_| to be called for
  // such event.
  //
  // This function is thread-safe.
  template<typename F>
  Optional<FlushEvent::Handle> on_flush(F&& callback_);

  // Query the name of the logger, which is the name given to the |RX_LOG| macro.
  const char* name() const;

  // Query the source information of where this log is defined.
  const SourceLocation& source_info() const &;

  void signal_write(Level _level, String&& contents_);
  void signal_flush();

private:
  const char* m_name;
  SourceLocation m_source_location;

  QueueEvent m_queue_event;
  WriteEvent m_write_event;
  FlushEvent m_flush_event;
};

inline constexpr Log::Log(const char* _name, const SourceLocation& _source_location)
  : m_name{_name}
  , m_source_location{_source_location}
{
}

template<typename... Ts>
bool Log::write(Level _level, const char* _format, Ts&&... _arguments) {
  if constexpr (sizeof...(Ts) > 0) {
    auto format = String::format(_format, Utility::forward<Ts>(_arguments)...);
    m_queue_event.signal(_level, {format.allocator(), format});
    return enqueue(this, _level, Utility::move(format));
  } else {
    m_queue_event.signal(_level, _format);
    return enqueue(this, _level, {_format});
  }
}

inline bool Log::write(Level _level, String&& message_) {
  m_queue_event.signal(_level, message_);
  return enqueue(this, _level, Utility::move(message_));
}

template<typename... Ts>
bool Log::warning(const char* _format, Ts&&... _arguments) {
  return write(Level::WARNING, _format, Utility::forward<Ts>(_arguments)...);
}

template<typename... Ts>
bool Log::info(const char* _format, Ts&&... _arguments) {
  return write(Level::INFO, _format, Utility::forward<Ts>(_arguments)...);
}

template<typename... Ts>
bool Log::verbose(const char* _format, Ts&&... _arguments) {
  return write(Level::VERBOSE, _format, Utility::forward<Ts>(_arguments)...);
}

template<typename... Ts>
bool Log::error(const char* _format, Ts&&... _arguments) {
  return write(Level::ERROR, _format, Utility::forward<Ts>(_arguments)...);
}

inline const char* Log::name() const {
  return m_name;
}

inline const SourceLocation& Log::source_info() const & {
  return m_source_location;
}

template<typename F>
inline Optional<Log::QueueEvent::Handle> Log::on_queue(F&& callback_) {
  if (auto fun = QueueEvent::Delegate::create(Utility::move(callback_))) {
    return m_queue_event.connect(Utility::move(*fun));
  }
  return nullopt;
}

template<typename F>
inline Optional<Log::WriteEvent::Handle> Log::on_write(F&& callback_) {
  if (auto fun = WriteEvent::Delegate::create(Utility::move(callback_))) {
    return m_write_event.connect(Utility::move(*fun));
  }
  return nullopt;
}

template<typename F>
inline Optional<Log::FlushEvent::Handle> Log::on_flush(F&& callback_) {
  if (auto fun = FlushEvent::Delegate::create(Utility::move(callback_))) {
    return m_flush_event.connect(Utility::move(*fun));
  }
  return nullopt;
}

#define RX_LOG(_name, _identifier) \
  static ::Rx::Global<::Rx::Log> _identifier{"loggers", (_name), (_name), \
    ::Rx::SourceLocation{__FILE__, "(global constructor)", __LINE__}}

} // namespace Rx

#endif // RX_CORE_LOG_H
