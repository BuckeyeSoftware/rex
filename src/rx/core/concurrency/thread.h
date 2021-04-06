#ifndef RX_CORE_CONCURRENCY_THREAD_H
#define RX_CORE_CONCURRENCY_THREAD_H
#include "rx/core/function.h"
#include "rx/core/ptr.h"

/// \file thread.h

namespace Rx::Concurrency {

/// \brief Manages a separate thread.
///
/// Represents a single thread of execution. Threads allow multiple functions to
/// execute concurrently.
///
/// Threads begin execution immediately upon creation by a successful call to
/// create(), starting at the top-level function provided to create(). The
/// return value of the top-level function is ignored.
///
/// Thread objects may be in the state that does not represent any thread (after
/// default construction, moved from, or join()).
///
/// \note Names must have static-storage duration.
/// \note On operating systems where signal-delivery is supported, all signals
/// will be blocked, thus signals cannot be delivered.
struct RX_API Thread {
  using Func = Function<void(Sint32)>;

  RX_MARK_NO_COPY(Thread);

  /// Constructs a thread which does not represent a thread.
  constexpr Thread();

  /// \brief Move constructor.
  ///
  /// Constructs the thread object to represent the thread of execution that was
  /// represented by \p thread_. After this call \p thread_ no longer represents
  /// a thread of execution.
  ///
  /// \param thread_ Another thread object to construct this thread object with.
  Thread(Thread&& thread_);

  /// \brief Destroys the thread object.
  ///
  /// \note A thread object does not have an associated thread (and is safe to
  /// destroy) after:
  ///  * It was default constructed.
  ///  * It was moved from.
  ///  * join() has been called.
  ~Thread();

  /// \brief Moves the thread object
  ///
  /// If \c *this still has an associated running thread, calls join().
  /// Then, or otherwise, assigns the state of \p thread_ to \c *this and sets
  /// \p thread_ to a default constructed state.
  ///
  /// \param thread_ Another thread object to assign this thread object.
  /// \returns \c *this
  Thread& operator=(Thread&& thread_);

  /// @{
  /// \brief Create a thread.
  ///
  /// Creates a new thread and associates it with a thread of execution.
  ///
  /// \param _allocator The allocator to allocate thread state with.
  /// \param _name The name to associate with this thread.
  /// \param function_ The callable object to execute in the new thread.
  /// \returns On successful construction, the Thread, otherwise nullopt.
  ///
  /// \note Any return value from \p function_ is ignored.
  /// \note Thread construction can fail if out of resources.
  /// \warning \p _name Must refer to a string that has static-storage duration,
  /// such as a string-literal.
  template<typename F>
  static Optional<Thread> create(Memory::Allocator& _allocator, const char* _name, F&& function_);
  static Optional<Thread> create(Memory::Allocator& _allocator, const char* _name, Func&& function_);
  /// @}

  /// \brief Waits for the thread to finish its execution.
  ///
  /// Blocks the current thread until the thread identified by \c *this finishes
  /// its execution.
  ///
  /// The completion of the thread identrified by \c *this _synchronizes_ with
  /// the corresponding successful join().
  ///
  /// \warning No synchronization is performed on \c *this itself. Concurrently
  /// calling join() on the same thread from multiple threads is a bug.
  [[nodiscard]] bool join();

private:
  struct RX_API State {
    RX_MARK_NO_COPY(State);
    RX_MARK_NO_MOVE(State);

    State(Memory::Allocator& _allocator, const char* _name, Func&& function_);

    [[nodiscard]] bool spawn();
    [[nodiscard]] bool join();

  private:
    static void* wrap(void* _data);

    union {
      struct {} m_nat;
      // Fixed-capacity storage for any OS thread Type, adjust if necessary.
      alignas(16) Byte m_thread[16];
    };

    Memory::Allocator& m_allocator;
    Func m_function;
    const char* m_name;
    bool m_joined;
  };

  // Called by non-templated |create|.
  Thread(Ptr<State>&& state_);

  Ptr<State> m_state;
};

// [Thread::State]
inline Thread::State::State(Memory::Allocator& _allocator, const char* _name, Func&& function_)
  : m_allocator{_allocator}
  , m_function{Utility::move(function_)}
  , m_name{_name}
  , m_joined{false}
{
}

// [Thread]
inline constexpr Thread::Thread() = default;

inline Thread::Thread(Thread&&) = default;

inline Thread::Thread(Ptr<State>&& state_)
  : m_state{Utility::move(state_)}
{
}

inline Thread::~Thread() {
  RX_ASSERT(join(), "thread not joined");
}

inline Thread& Thread::operator=(Thread&& thread_) {
  if (this != &thread_) {
    // This join can only fail if the thread crashed.
    RX_ASSERT(join(), "failed to join");
    m_state = Utility::move(thread_.m_state);
  }
  return *this;
}

template<typename F>
Optional<Thread> Thread::create(Memory::Allocator& _allocator, const char* _name, F&& function_) {
  auto function = Func::create(Utility::forward<F>(function_));
  if (!function) {
    return nullopt;
  }
  return create(_allocator, _name, Utility::move(*function));
}

} // namespace Rx::Concurrency

#endif // RX_CORE_CONCURRENCY_THREAD_H
