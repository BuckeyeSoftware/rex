#include "rx/core/concurrency/thread.h"

#include "rx/core/concurrency/atomic.h"
#include "rx/core/memory/system_allocator.h"
#include "rx/core/string.h"
#include "rx/core/profiler.h"

#if defined(RX_PLATFORM_POSIX)
#include <pthread.h> // pthread_t
#include <signal.h> // sigset_t, setfillset
#elif defined(RX_PLATFORM_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h> // HANDLE
#include <process.h> // _beginthreadex
#else
#error "missing thread implementation"
#endif

namespace Rx::Concurrency {

static Atomic<int> g_thread_id;

#if defined(RX_PLATFORM_WINDOWS)
struct ThreadDescription {
  ThreadDescription()
    : m_set_thread_description{nullptr}
  {
    // Determine if Windows supports the new SetThreadDescription API.
    auto kernel = GetModuleHandleA("kernel32.dll");
    auto set_thread_description = GetProcAddress(kernel, "SetThreadDescription");
    if (set_thread_description) {
      *reinterpret_cast<void**>(&m_set_thread_description) = set_thread_description;
    }
  }

  void set(HANDLE _handle, const String& _name) {
    // When we have the new SetThreadDescription API, use it.
    if (m_set_thread_description) {
      // Convert thread name to UTF-16.
      const WideString name = _name.to_utf16();
      m_set_thread_description(_handle, reinterpret_cast<PCWSTR>(name.data()));
    } else if (IsDebuggerPresent()) {
      // Fallback is only supported inside the debugger.
      #pragma pack(push, 8)
      typedef struct THREAD_NAME_INFO {
        DWORD dwType;
        LPCSTR szName;
        DWORD dwThreadID;
        DWORD dwFlags;
      } THREAD_NAME_INFO;
      #pragma pack(pop)

      THREAD_NAME_INFO info;

      info.dwType = 0x1000;
      info.szName = _name.data();
      info.dwThreadID = GetThreadId(_handle);
      info.dwFlags = 0;

      #pragma warning(push)
      #pragma warning(disable: 6320 6322)
        __try {
          RaiseException(0x406D1388, 0, sizeof(THREAD_NAME_INFO) / sizeof(ULONG_PTR),
            reinterpret_cast<ULONG_PTR*>(&info));
        } __except (EXCEPTION_EXECUTE_HANDLER) {
          // Do nothing.
        }
      #pragma warning(pop)
    }
  }

private:
  HANDLE (WINAPI* m_set_thread_description)(HANDLE, PCWSTR);
};

static Global<ThreadDescription> s_thread_description{"system", "thread_description"};
#endif // !defined(RX_PLATFORM_WINDOWS)

// [Thread]
Optional<Thread> Thread::create(Memory::Allocator& _allocator, const char* _name, Func&& function_) {
  auto state = make_ptr<State>(_allocator, _name, Utility::move(function_));
  if (state && state->spawn()) {
    return Thread{Utility::move(state)};
  }
  return nullopt;
}

bool Thread::join() {
  // When there is no state the join should always succeed.
  if (m_state) {
    return m_state->join();
  }
  return true;
}

// [Thread::State]
void* Thread::State::wrap(void* _data) {
#if defined(RX_PLATFORM_POSIX) && !defined(RX_PLATFORM_EMSCRIPTEN)
  // Don't permit any signal delivery to threads.
  sigset_t mask;
  sigfillset(&mask);
  RX_ASSERT(pthread_sigmask(SIG_BLOCK, &mask, nullptr) == 0,
    "failed to block signals");
#endif

  // Record the thread name into the global profiler.
  auto self = reinterpret_cast<State*>(_data);
  Profiler::instance().set_thread_name(self->m_name);

  // Dispatch the actual thread function.
  self->m_function(g_thread_id++);

  return nullptr;
}

bool Thread::State::spawn() {
#if defined(RX_PLATFORM_POSIX)
  // Spawn the thread.
  auto handle = reinterpret_cast<pthread_t*>(m_thread);
  if (pthread_create(handle, nullptr, wrap, reinterpret_cast<void*>(this)) != 0) {
    return false;
  }

#if !defined(RX_PLATFORM_EMSCRIPTEN)
  // Set the thread's name.
  pthread_setname_np(*handle, m_name);
#endif

  return true;
#elif defined(RX_PLATFORM_WINDOWS)
  // |_beginthreadex| is a bit non-standard in that it expects the __stdcall
  // calling convention and to return unsigned int status. Our |wrap| function
  // is more traditional in that it returns void* and uses the same calling
  // convention as default functions. Here we produce a lambda function which
  // will wrap |wrap|.
  auto wrap_win32 = [](void* _data) -> unsigned {
    wrap(_data);
    return 0;
  };

  // Spawn the thread.
  auto thread = _beginthreadex(nullptr, 0,
    wrap_win32, reinterpret_cast<void*>(this), 0, nullptr);

  if (!thread) {
    return false;
  }

  auto handle = reinterpret_cast<HANDLE>(thread);

  // Set the thread description.
  s_thread_description->set(handle, *String::create(Memory::SystemAllocator::instance(), m_name));

  // Store the handle.
  *reinterpret_cast<HANDLE*>(m_thread) = handle;

  return true;

#endif
  return false;
}

bool Thread::State::join() {
  if (m_joined) {
    return true;
  }

#if defined(RX_PLATFORM_POSIX)
  auto handle = *reinterpret_cast<pthread_t*>(m_thread);
  if (pthread_join(handle, nullptr) != 0) {
    return false;
  }
#elif defined(RX_PLATFORM_WINDOWS)
  auto handle = *reinterpret_cast<HANDLE*>(m_thread);

  // Wait for the thread to terminate.
  if (WaitForSingleObject(handle, INFINITE) != WAIT_OBJECT_0) {
    return false;
  }

  // Destroy the thread object itself.
  CloseHandle(handle);
#endif

  m_joined = true;

  return true;
}

} // namespace Rx::Concurrency
