#ifndef RX_CORE_CONCURRENCY_THREAD_H
#define RX_CORE_CONCURRENCY_THREAD_H
#include "rx/core/function.h"
#include "rx/core/ptr.h"

namespace Rx::Concurrency {

// NOTE: Thread names must have static-storage which lives as long as the thread.
// NOTE: Cannot deliver signals to threads.
struct RX_API Thread {
  using Func = Function<void(Sint32)>;

  RX_MARK_NO_COPY(Thread);

  constexpr Thread();
  Thread(Thread&& thread_);
  ~Thread();
  Thread& operator=(Thread&& thread_);

  template<typename F>
  static Optional<Thread> create(Memory::Allocator& _allocator, const char* _name, F&& function_);
  // Called by above function.
  static Optional<Thread> create(Memory::Allocator& _allocator, const char* _name, Func&& func_);

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
