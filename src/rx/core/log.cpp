#include <time.h> // time_t, tm, strftime
#include <string.h> // strlen

#include "rx/core/log.h" // log
#include "rx/core/string.h" // string
#include "rx/core/vector.h" // vector
#include "rx/core/profiler.h" // g_profiler
#include "rx/core/algorithm/max.h"

#include "rx/core/filesystem/file.h" // file

#include "rx/core/concurrency/mutex.h" // mutex
#include "rx/core/concurrency/condition_variable.h" // condition_variable
#include "rx/core/concurrency/scope_lock.h" // scope_lock
#include "rx/core/concurrency/scope_unlock.h" // scope_unlock
#include "rx/core/concurrency/thread.h" // thread

#include "rx/core/global.h"

namespace rx {

static global_group g_group_loggers{"loggers"};

global<Logger> Logger::s_logger{"system", "logger"};

static inline const char* string_for_level(log::level _level) {
  switch (_level) {
  case log::level::k_warning:
    return "warning";
  case log::level::k_info:
    return "info";
  case log::level::k_verbose:
    return "verbose";
  case log::level::k_error:
    return "error";
  }
  return nullptr;
}

static inline string string_for_time(time_t _time) {
  struct tm tm;
#if defined(RX_PLATFORM_WINDOWS)
  localtime_s(&tm, &_time);
#else
  localtime_r(&_time, &tm);
#endif
  char date[256];
  strftime(date, sizeof date, "%Y-%m-%d %H:%M:%S", &tm);
  date[sizeof date - 1] = '\0';
  return date;
}

Logger::Logger()
  : m_status{k_running}
  , m_padding{0}
  , m_thread{"logger", [this](int _thread_id) { process(_thread_id); }}
{
  // Calculate padding needed for formatting log level.
  int max_level = algorithm::max(
    strlen(string_for_level(log::level::k_warning)),
    strlen(string_for_level(log::level::k_info)),
    strlen(string_for_level(log::level::k_verbose)),
    strlen(string_for_level(log::level::k_error)));

  int max_name = 0;
  g_group_loggers.each([&](global_node* _node) {
    // Initialize the logger.
    _node->init();

    // Associate a message queue with the logger.
    auto this_log = _node->cast<log>();
    m_queues.emplace_back(this_log, intrusive_list{});

    // Keep track of the largest logger name.
    const auto length = strlen(this_log->name());
    max_name = algorithm::max(max_name, static_cast<int>(length));
  });

  // The padding needed is the sum of the largest level and name strings + 1.
  m_padding = max_level + max_name + 1;

  // Wakeup |process| thread.
  {
    concurrency::scope_lock lock{m_mutex};
    m_status |= k_ready;
    m_ready_cond.signal();
  }
}

Logger::~Logger() {
  // Signal the |process| thread to terminate.
  {
    concurrency::scope_lock lock{m_mutex};
    m_status &= ~k_running;
    m_wakeup_cond.signal();
  }

  // Join the |process| thread.
  m_thread.join();

  // Finalize all loggers.
  g_group_loggers.fini();
}

bool Logger::subscribe(stream* _stream) {
  concurrency::scope_lock lock{m_mutex};
  if (const auto find = m_streams.find(_stream); find != -1_z) {
    return false;
  }

  return m_streams.push_back(_stream);
}

bool Logger::unsubscribe(stream* _stream) {
  concurrency::scope_lock lock{m_mutex};
  if (const auto find = m_streams.find(_stream); find != -1_z) {
    m_streams.erase(find, find + 1);
    return true;
  }
  return false;
}

bool Logger::enqueue(log* _owner, log::level _level, string&& message_) {
  concurrency::scope_lock lock{m_mutex};

  const auto index = m_queues.find_if([_owner](const queue& _queue) {
    return _queue.owner == _owner;
  });

  if (index != -1_z) {
    auto& this_queue = m_queues[index];

    // Record the message.
    auto this_message = make_ptr<message>(&memory::g_system_allocator,
      &this_queue, _level, time(nullptr), utility::move(message_),
      intrusive_list::node{});

    if (!this_message || !m_messages.emplace_back(utility::move(this_message))) {
      return false;
    }

    // Record the link.
    this_queue.messages.push_back(&m_messages.last()->link);

    // Wakeup logging thread when we have a few messages.
    if (m_streams.size() && m_messages.size() >= 1000) {
      m_wakeup_cond.signal();
    }

    return true;
  }

  return false;
}

void Logger::process([[maybe_unused]] int _thread_id) {
  concurrency::scope_lock locked{m_mutex};

  // Block the logging thread until |this| is ready.
  m_ready_cond.wait(locked, [this] { return m_status & k_ready; });

  while (m_status & k_running) {
    // Block until another we're woken up again to flush something.
    m_wakeup_cond.wait(locked);

    // Flush the queued contents.
    flush();
  }
}

void Logger::flush() {
  // Flush all message entries and clear them.
  m_messages.each_fwd([this](ptr<message>& message_) {
    write(message_);
  });
  m_messages.clear();
}

void Logger::write(ptr<message>& message_) {
  auto this_queue = message_->owner;

  const auto name = this_queue->owner->name();
  const auto level = string_for_level(message_->level);
  const auto padding = strlen(name) + strlen(level) + 1; // +1 for '/'

  // The streams written to are all binary streams. Handle platform differences
  // for handling for newline.
  const char* format =
#if defined(RX_PLATFORM_WINDOWS)
    "[%s] [%s/%s]%*s | %s\r\n";
#else
    "[%s] [%s/%s]%*s | %s\n";
#endif

  const auto contents = string::format(
    format,
    string_for_time(message_->time),
    name,
    level,
    m_padding - padding,
    "",
    message_->contents);

  // Send formatted message to each stream.
  m_streams.each_fwd([&contents](stream* _stream) {
    const auto data = reinterpret_cast<const rx_byte*>(contents.data());
    const auto size = contents.size();
    RX_ASSERT(_stream->write(data, size) != 0, "failed to write to stream");
  });

  // Signal the write event for the log associated with this message.
  this_queue->owner->signal_write(message_->level,
    utility::move(message_->contents));

  // Remove the message from the queue for the log associated with this message.
  this_queue->messages.erase(&message_->link);

  // The queue for the log associated with this message is now empty.
  if (this_queue->messages.is_empty()) {
    // Signal the flush operation on that log.
    this_queue->owner->signal_flush();
  }
}

void log::signal_write(level _level, string&& contents_) {
  // NOTE(dweiler): This is called by the logging thread.
  m_write_event.signal(_level, utility::move(contents_));
}

void log::signal_flush() {
  // NOTE(dweiler): This is called by the logging thread.
  m_flush_event.signal();
}

} // namespace rx
