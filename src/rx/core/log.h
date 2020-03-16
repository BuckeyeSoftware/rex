#ifndef RX_CORE_LOG_H
#define RX_CORE_LOG_H
#include <time.h> // time_t

#include "rx/core/global.h"
#include "rx/core/string.h"
#include "rx/core/event.h"
#include "rx/core/ptr.h"
#include "rx/core/intrusive_list.h"

#include "rx/core/concurrency/mutex.h"
#include "rx/core/concurrency/thread.h"
#include "rx/core/concurrency/condition_variable.h"
#include "rx/core/concurrency/scope_lock.h"

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

  log(const char* _name, const char* _file_name, int _line);

  // Write a formatted message given by |_format| and |_arguments| of associated
  // severity level |_level|. This will queue the given message on the logger
  // with |logger::enqueue|. The delegates given by |on_queue| will be called
  // immediately by the same thread which calls this.
  //
  // This function can fail if the logger instance |logger::enqueue| fails.
  //
  // This function is thread-safe.
  template<typename... Ts>
  bool write(level _level, const char* _format, Ts&&... _arguments);

  // Convenience functions that call |write| with the associated levels:
  // warning, info, verbose, error.
  //
  // These functions can fail if the logger instance |logger::enqueue| fails.
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

  const char* name() const;
  const char* file_name() const;
  int line() const;

  // When a message is queued, any delegates associated by this function are
  // called. This is different from |on_write| in that |callback_| is called
  // by the same thread which calls |write|, |warning|, |info|, |verbose|, or
  // |error| immediately.
  //
  // This function returns the event handle, keep the handle alive for as
  // long as you want the delegate |callback_| to be called.
  //
  // This function is thread-safe.
  queue_event::handle on_queue(queue_event::delegate&& callback_);

  // When a message is written, any delegates associated by this function are
  // called. This is different from |on_queue| in that |callback_| is called
  // by the logging thread when the actual message is written.
  //
  // This function returns the event handle, keep the handle alive for as
  // long as you want the delegate |callback_| to be called.
  //
  // This function is thread-safe.
  write_event::handle on_write(write_event::delegate&& callback_);

  // Multiple messages associated with this log can be queued up before they're
  // written by the loggger, when all the queued messages are written out, any
  // delegate associated by this function are callled.
  //
  // This function returns the event handle, keep the handle alive for as
  // long as you want the delegate |callback_| to be called.
  //
  // This function is thread-safe.
  flush_event::handle on_flush(flush_event::delegate&& callback_);

private:
  friend struct Logger;

  void signal_write(level _level, string&& contents_);
  void signal_flush();

  const char* m_name;
  const char* m_file_name;
  int m_line;

  queue_event m_queue_event;
  write_event m_write_event;
  flush_event m_flush_event;
};

struct Logger {
  Logger();
  ~Logger();

  // The single global logger instance.
  static Logger& instance();

  // Subscribe and unsubscribe a given stream on the logger.
  //
  // Every stream acts as a destination for formatted log messages to go to,
  // this can be anything which implements the stream interface, including files,
  // mmaped data, network, etc.
  //
  // The stream must support writing, |stream::can_write|. This function returns
  // false if the stream does not support writing or the stream |_stream| is
  // allready associated with the logger, true otherwise.
  //
  // This function is thread-safe.
  [[nodiscard]] bool subscribe(stream* _stream);

  // Unsubscribe a stream |_stream| from the logger. No more log messages will
  // go to the stream when ubsubscribed. This function returns false if the
  // stream |_stream| isn't subscribed, true otherwise.
  //
  // This function is thread-safe.
  [[nodiscard]] bool unsubscribe(stream* _stream);

  // Enqueue a log message on |m_queue|, will be processed by |process| eventually.
  //
  // This can be used to log a message directly on the logger without having
  // an rx::log instance.
  //
  // This function returns false if the message could not be queued on the
  // logger, which indicates an OOM, true otherwise.
  //
  // This function is thread-safe.
  [[nodiscard]] bool enqueue(log* _log, log::level _level, string&& _message);

private:
  enum {
    k_running = 1 << 0,
    k_ready   = 1 << 1
  };

  struct queue {
    log* owner;
    intrusive_list messages;
  };

  struct message {
    queue* owner;
    log::level level;
    time_t time;
    string contents;
    intrusive_list::node link;
  };

  void process(int _thread_id);

  void flush();
  void write(ptr<message>& message_);

  concurrency::mutex m_mutex;
  concurrency::condition_variable m_ready_cond;
  concurrency::condition_variable m_wakeup_cond;

  vector<stream*> m_streams;       // protected by |m_mutex|
  vector<queue> m_queues;          // protected by |m_mutex|
  vector<ptr<message>> m_messages; // protected by |m_mutex|
  int m_status;                    // protected by |m_mutex|
  int m_padding;                   // protected by |m_mutex|

  // NOTE(dweiler): This should come last.
  concurrency::thread m_thread;

  static global<Logger> s_logger;
};

inline log::log(const char* _name, const char* _file_name, int _line)
  : m_name{_name}
  , m_file_name{_file_name}
  , m_line{_line}
{
}

template<typename... Ts>
inline bool log::write(level _level, const char* _format, Ts&&... _arguments) {
  auto format = string::format(_format, utility::forward<Ts>(_arguments)...);
  m_queue_event.signal(_level, format);
  return Logger::instance().enqueue(this, _level, utility::move(format));
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

inline const char* log::file_name() const {
  return m_file_name;
}

inline int log::line() const {
  return m_line;
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

inline Logger& Logger::instance() {
  return *s_logger;
};

#define RX_LOG(_name, _identifier) \
  static ::rx::global<::rx::log> _identifier{"loggers", (_name), (_name), __FILE__, __LINE__}

} // namespace rx

#endif // RX_CORE_LOG_H
