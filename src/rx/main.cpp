#include <signal.h> // signal, sig_atomic_t, SIG{INT,TERM,HUP,QUIT,KILL,PIPE,ALRM,STOP}

#include "rx/engine.h"

#if defined(RX_PLATFORM_EMSCRIPTEN)
#include <emscripten.h>
#include <emscripten/html5.h>
#endif // defined(RX_PLATFORM_EMSCRIPTEN)

[[maybe_unused]] static volatile sig_atomic_t g_running = 1;

int main([[maybe_unused]] int _argc, [[maybe_unused]] char** argv) {
  // Link all globals into their respective groups.
  Rx::Globals::link();

  // Initialize all globals now. Any order dependence will be handled entierly
  // by the globals system. The order of initialization is recorded so that
  // finalization occurs in reverse initialization order.
  Rx::Globals::init();

  // Emscripten does not allow us to block in main as that blocks the browser
  // and prevents worker threads from being posted. Instead let the browser
  // drive our main loop with RAF (Request Animation Frame).
  //
  // Similarly, use of WebGL outside of RAF before the main loop is registered
  // with Emscripten isn't permitted either. Engine initialization executes
  // some WebGL functions for setting things up so these need to be moved to
  // RAF as well.
  //
  // The solution used here is to have Emscripten execute a loop function which
  // monkey patches itself once the engine is initialized. This avoids checking
  // for initialization every frame.
#if defined(RX_PLATFORM_EMSCRIPTEN)
  struct Environment {
    Environment()
      : m_loop{init_loop}
      {
      }

    void loop() {
      m_loop(this);
    }

  private:
    Rx::Engine m_engine;
    void (*m_loop)(Environment* _environment);

    static void init_loop(Environment* _environment) {
      if (!_environment->m_engine.init()) {
        emscripten_cancel_main_loop();
      } else {
        _environment->m_loop = run_loop;
      }
    }

    static void run_loop(Environment* _environment) {
      const auto status = _environment->m_engine.run();
      if (status != Rx::Engine::Status::RUNNING) {
        emscripten_cancel_main_loop();
      }
    }
  };

  Environment environment;
  auto loop = [](void* _environment) {
    reinterpret_cast<Environment*>(_environment)->loop();
  };
  emscripten_set_main_loop_arg(loop, static_cast<void*>(&environment), 0, 1);
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

  // So that engine is destroyed before fini is called.
  {
    // Restart is just a matter of goto.
  restart:
    Rx::Engine engine;
    if (!engine.init()) {
      return 1;
    }

    while (g_running) {
      switch (engine.run()) {
      case Rx::Engine::Status::RESTART:
        goto restart;
      case Rx::Engine::Status::SHUTDOWN:
        return 0;
      case Rx::Engine::Status::RUNNING:
        break;
      }
    }
  }
#endif

  // Destroy all initialized globals now. The globals system keeps track of which
  // globals are initialized and in which order. This will deinitialize them in
  // reverse order for only the ones that are actively initialized.
  Rx::Globals::fini();

  return 0;
}
