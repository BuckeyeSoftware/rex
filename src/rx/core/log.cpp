#include <time.h> // time_t, time
#include <string.h> // strlen

#include "rx/core/log.h"
#include "rx/core/ptr.h"
#include "rx/core/intrusive_list.h"

#include "rx/core/stream/untracked_stream.h"
#include "rx/core/stream/buffered_stream.h"

#include "rx/core/algorithm/max.h"

#include "rx/core/concurrency/mutex.h"
#include "rx/core/concurrency/condition_variable.h"
#include "rx/core/concurrency/thread.h"

#if defined(RX_PLATFORM_EMSCRIPTEN)
#include <emscripten.h>
#endif

namespace Rx {

namespace {

struct Logger {
  Logger();
  ~Logger();

  static constexpr Logger& instance();

  bool subscribe(Stream::UntrackedStream& _stream);
  bool unsubscribe(Stream::UntrackedStream& _stream);
  bool enqueue(Log* _log, Log::Level _level, String&& _message);
  void flush();

private:
  enum : Uint8 {
    RUNNING = 1 << 0,
    READY   = 1 << 1
  };

  struct Queue {
    Log* owner;
    IntrusiveList messages;
  };

  struct Message {
    Queue* owner;
    Log::Level level;
    time_t time;
    String contents;
    IntrusiveList::Node link;
  };

  void process(int _thread_id);

  void flush_unlocked();
  void write(Ptr<Message>& message_);

  Concurrency::Mutex m_mutex;
  Concurrency::ConditionVariable m_ready_cond;
  Concurrency::ConditionVariable m_wakeup_cond;

  Vector<Stream::BufferedStream> m_streams RX_HINT_GUARDED_BY(m_mutex);
  Vector<Queue> m_queues                   RX_HINT_GUARDED_BY(m_mutex);
  Vector<Ptr<Message>> m_messages          RX_HINT_GUARDED_BY(m_mutex);
  int m_status                             RX_HINT_GUARDED_BY(m_mutex);
  int m_padding                            RX_HINT_GUARDED_BY(m_mutex);

  // NOTE(dweiler): This should come last.
  Concurrency::Thread m_thread;

  static Global<Logger> s_instance;
};

static GlobalGroup g_group_loggers{"loggers"};

Global<Logger> Logger::s_instance{"system", "logger"};

static inline const char* string_for_level(Log::Level _level) {
  switch (_level) {
  case Log::Level::WARNING:
    return "warning";
  case Log::Level::INFO:
    return "info";
  case Log::Level::VERBOSE:
    return "verbose";
  case Log::Level::ERROR:
    return "error";
  }
  return nullptr;
}

static inline String string_for_time(time_t _time) {
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
  : m_streams{Memory::SystemAllocator::instance()}
  , m_queues{Memory::SystemAllocator::instance()}
  , m_messages{Memory::SystemAllocator::instance()}
  , m_status{RUNNING}
  , m_padding{0}
  , m_thread{}
{
  auto thread = Concurrency::Thread::create(
    Memory::SystemAllocator::instance(),
    "loggger", [this](int _tid) { process(_tid); });

  RX_ASSERT(thread, "failed to create thread");

  m_thread = Utility::move(*thread);

  // Calculate padding needed for formatting log level.
  int max_level = Algorithm::max(
    strlen(string_for_level(Log::Level::WARNING)),
    strlen(string_for_level(Log::Level::INFO)),
    strlen(string_for_level(Log::Level::VERBOSE)),
    strlen(string_for_level(Log::Level::ERROR)));

  int max_name = 0;
  g_group_loggers.each([&](GlobalNode* _node) {
    // Initialize the logger.
    _node->init();

    // Associate a message queue with the logger.
    auto this_log = _node->cast<Log>();
    m_queues.emplace_back(this_log, IntrusiveList{});

    // Keep track of the largest logger name.
    const auto length = strlen(this_log->name());
    max_name = Algorithm::max(max_name, static_cast<int>(length));
  });

  // The padding needed is the sum of the largest level and name strings + 1.
  m_padding = max_level + max_name + 1;

  // Wakeup |process| thread.
  {
    Concurrency::ScopeLock lock{m_mutex};
    m_status |= READY;
    m_ready_cond.signal();
  }
}

Logger::~Logger() {
  // Signal the |process| thread to terminate.
  {
    Concurrency::ScopeLock lock{m_mutex};
    m_status &= ~RUNNING;
    m_wakeup_cond.signal();
  }

  // Join the |process| thread.
  RX_ASSERT(m_thread.join(), "failed to join thread");

  // Finalize all loggers.
  g_group_loggers.fini();
}

inline constexpr Logger& Logger::instance() {
  return *s_instance;
}

bool Logger::subscribe(Stream::UntrackedStream& _stream) {
  // The stream needs to be writable.
  if (!(_stream.flags() & Stream::WRITE)) {
    return false;
  }

  auto& allocator = Memory::SystemAllocator::instance();

  auto stream = Stream::BufferedStream::create(allocator);
  if (!stream || !stream->attach(_stream)) {
    return false;
  }

  auto compare = [&](const Stream::BufferedStream& _buffered_stream) {
    return _buffered_stream.stream() == &_stream;
  };

  Concurrency::ScopeLock lock{m_mutex};

  // Don't allow subscribing the same stream more than once.
  if (const auto find = m_streams.find_if(compare)) {
    return false;
  }

  return m_streams.push_back(Utility::move(*stream));
}

bool Logger::unsubscribe(Stream::UntrackedStream& _stream) {
  Concurrency::ScopeLock lock{m_mutex};

  auto compare = [&](const Stream::BufferedStream& _buffered_stream) {
    return _buffered_stream.stream() == &_stream;
  };

  if (const auto find = m_streams.find_if(compare)) {
    // Flush any contents when removing a stream from the logger.
    flush_unlocked();
    m_streams.erase(*find, *find + 1);
    return true;
  }
  return false;
}

bool Logger::enqueue(Log* _owner, Log::Level _level, String&& message_) {
  Concurrency::ScopeLock lock{m_mutex};

  const auto index = m_queues.find_if([_owner](const Queue& _queue) {
    return _queue.owner == _owner;
  });

  if (!index) {
    return false;
  }

  auto& this_queue = m_queues[*index];

  // Record the message.
  auto this_message = make_ptr<Message>(Memory::SystemAllocator::instance(),
                                        &this_queue, _level, time(nullptr), Utility::move(message_),
                                        IntrusiveList::Node{});

  if (!this_message || !m_messages.emplace_back(Utility::move(this_message))) {
    return false;
  }

  // Record the link.
  this_queue.messages.push_back(&m_messages.last()->link);

  // Wakeup logging thread when we have a few messages.
#if defined(RX_DEBUG)
  static constexpr const auto FLUSH_THRESHOLD = 1_z;
#else
  static constexpr const auto FLUSH_THRESHOLD = 1000_z;
#endif

  if (m_streams.size() && m_messages.size() >= FLUSH_THRESHOLD) {
    m_wakeup_cond.signal();
  }

  return true;
}

void Logger::flush() {
  Concurrency::ScopeLock lock{m_mutex};
  flush_unlocked();
}

void Logger::process([[maybe_unused]] int _thread_id) {
  Concurrency::ScopeLock locked{m_mutex};

  // Block the logging thread until |this| is ready.
  m_ready_cond.wait(locked, [this] { return m_status & READY; });

  while (m_status & RUNNING) {
    // Block until another we're woken up again to flush something.
    m_wakeup_cond.wait(locked);

    // Flush the queued contents. Use the unlocked variant since |m_mutex| is
    // held by |m_wakeup_cond|.
    flush_unlocked();
  }
}

void Logger::flush_unlocked() {
  // Flush all message entries.
  m_messages.each_fwd([this](Ptr<Message>& message_) { write(message_); });
  m_messages.clear();

  // Flush all the streams.
  m_streams.each_fwd([](Stream::BufferedStream& _stream) {
    _stream.on_flush();
  });
}

void Logger::write(Ptr<Message>& message_) {
  auto this_queue = message_->owner;

  const auto name = this_queue->owner->name();
  const auto level = string_for_level(message_->level);
  const auto padding = strlen(name) + strlen(level) + 1; // +1 for '/'

  // The streams written to are all binary streams. Handle platform differences
  // for handling for newline.
  const auto contents = String::format(
    Memory::SystemAllocator::instance(),
    "[%s] [%s/%s]%*s | ",
    string_for_time(message_->time),
    name,
    level,
    m_padding - padding,
    "");

  // Send formatted message to each stream.
  const auto& message = message_->contents;

#if defined(RX_PLATFORM_WINDOWS)
  static constexpr const char CR[] = {'\r', '\n'};
#else
  static constexpr const char CR[] = {'\n'};
#endif

  m_streams.each_fwd([&](Stream::BufferedStream& _stream) {
    auto stat = _stream.on_stat();
    auto offset = stat->size;
    for (auto beg = &message.first(); beg < &message.last() + 1; ) {
      offset += _stream.on_write((const Byte*)contents.data(), contents.size(), offset);
      auto end = strchr(beg, '\n');
      if (!end) {
        end = &message.last() + 1;
      }
      offset += _stream.on_write((const Byte*)beg, end - beg, offset);
      offset += _stream.on_write((const Byte*)CR, sizeof CR, offset);
      beg = end + 1;
    }
  });

  // Write the log to the browser for Emscripten.
  //
  // TODO(dweiler): Implement a Stream::TrackedStream for Emscripten log and
  // have engine entry point attach it to the logger. This should not be in
  // here.
#if defined(RX_PLATFORM_EMSCRIPTEN)
  switch (message_->level) {
  case Log::Level::ERROR:
    emscripten_log(EM_LOG_ERROR, "%s", contents.data());
    break;
  case Log::Level::INFO:
    [[fallthrough]];
  case Log::Level::VERBOSE:
    emscripten_log(EM_LOG_INFO, "%s", contents.data());
    break;
  case Log::Level::WARNING:
    emscripten_log(EM_LOG_WARN, "%s", contents.data());
    break;
  }
#endif

  // Signal the write event for the log associated with this message.
  this_queue->owner->signal_write(message_->level,
                                  Utility::move(message_->contents));

  // Remove the message from the log's queue associated with this message.
  this_queue->messages.erase(&message_->link);

  // The queue for the log associated with this message is now empty.
  if (this_queue->messages.is_empty()) {
    // Signal the flush operation on that log, to indicate any messages
    // queued up on it are now all written out.
    this_queue->owner->signal_flush();
  }
}

} // anon-namespace

Log::Log(const char* _name, const SourceLocation& _source_location)
  : m_name{_name}
  , m_source_location{_source_location}
  , m_queue_event{Memory::SystemAllocator::instance()}
  , m_write_event{Memory::SystemAllocator::instance()}
  , m_flush_event{Memory::SystemAllocator::instance()}
{
}

void Log::signal_write(Level _level, String&& contents_) {
  // NOTE(dweiler): This is called by the logging thread.
  m_write_event.signal(_level, Utility::move(contents_));
}

void Log::signal_flush() {
  // NOTE(dweiler): This is called by the logging thread.
  m_flush_event.signal();
}

bool Log::enqueue(Log* _owner, Level _level, String&& contents_) {
  return Logger::instance().enqueue(_owner, _level, Utility::move(contents_));
}

void Log::flush() {
  Logger::instance().flush();
}

bool Log::subscribe(Stream::UntrackedStream& _stream) {
  return Logger::instance().subscribe(_stream);
}

bool Log::unsubscribe(Stream::UntrackedStream& _stream) {
  return Logger::instance().unsubscribe(_stream);
}

} // namespace Rx
