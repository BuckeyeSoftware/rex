#include <signal.h> // signal, sig_atomic_t, SIG{INT,TERM,HUP,QUIT,KILL,PIPE,ALRM,STOP}
#include <SDL.h>

#include "rx/engine.h"

#include "rx/core/filesystem/unbuffered_file.h"
#include "rx/core/log.h"

#if defined(RX_PLATFORM_EMSCRIPTEN)
#include <emscripten.h>
#endif // defined(RX_PLATFORM_EMSCRIPTEN)

[[maybe_unused]] static volatile sig_atomic_t g_running = 1;

int main([[maybe_unused]] int _argc, [[maybe_unused]] char** argv) {
  // Link all globals into their respective groups.
  if (!Rx::Globals::link()) {
    return 1;
  }

  // Initialize logger as early as possible.
  Rx::Globals::find("system")->find("logger")->init();

  auto log = Rx::Filesystem::UnbufferedFile::open(Rx::Memory::SystemAllocator::instance(), "log.log", "w");
  if (!log || !Rx::Log::subscribe(*log)) {
    return -1;
  }

#if defined(RX_PLATFORM_EMSCRIPTEN)
  Rx::Engine engine;
  auto loop = [](void* _engine) {
    auto engine = reinterpret_cast<Rx::Engine*>(_engine);
    const auto status = engine->run();
    if (status != Rx::Engine::Status::RUNNING) {
      emscripten_cancel_main_loop();
    }
  };

  auto init = [](void* _engine) {
    auto engine = reinterpret_cast<Rx::Engine*>(_engine);
    if (!engine->init()) {
      emscripten_force_exit(-1);
    }
  };

  // This is a blocker before the main loop can run.
  emscripten_push_main_loop_blocker(init, static_cast<void*>(&engine));

  // Start the main loop.
  emscripten_set_main_loop_arg(loop, static_cast<void*>(&engine), 0, 1);

#else
  auto catch_signal = [](int) { g_running = 0; };

  signal(SIGINT, catch_signal);
  signal(SIGTERM, catch_signal);

  // Don't catch these signals in debug.
#if !defined(RX_DEBUG)
  signal(SIGILL, catch_signal);
  signal(SIGABRT, catch_signal);
  signal(SIGFPE, catch_signal);
  signal(SIGSEGV, catch_signal);
#endif

  // Widnows does not have these signals.
#if !defined(RX_PLATFORM_WINDOWS)
  signal(SIGHUP, catch_signal);
  signal(SIGQUIT, catch_signal);
  signal(SIGKILL, catch_signal);
  signal(SIGPIPE, catch_signal);
  signal(SIGALRM, catch_signal);
  signal(SIGSTOP, catch_signal);
#endif // !defined(RX_PLATFORM_WINDOWS)

  for (;;) {
    Rx::Engine engine;
    if (!engine.init()) {
      break;
    }

    Rx::Engine::Status status = Rx::Engine::Status::SHUTDOWN;
    while (g_running) {
      status = engine.run();
      if (status != Rx::Engine::Status::RUNNING) {
        break;
      }
    }

    if (status == Rx::Engine::Status::SHUTDOWN || !g_running) {
      break;
    }
  }
#endif

  if (!Rx::Log::unsubscribe(*log)) {
    // Shouldn't fail but if it does, flush the logger.
    Rx::Log::flush();
  }

  // Destroy all initialized globals now. The globals system keeps track of
  // which globals are initialized and in which order. This will deinitialize
  // them in reverse order for only the ones that are actively initialized.
  Rx::Globals::fini();

  return 0;
}
