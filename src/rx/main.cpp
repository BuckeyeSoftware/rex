#include <signal.h> // signal, sig_atomic_t, SIG{INT,TERM,HUP,QUIT,KILL,PIPE,ALRM,STOP}
#include <stdio.h>
#include <stdlib.h> // malloc

#include "rx/engine.h"

static volatile sig_atomic_t g_running = 1;

int main(int _argc, char** _argv) {
  (void)_argc;
  (void)_argv;

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

  // Link all globals into their respective groups.
  Rx::Globals::link();

  // Restarting is a goto!.
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

  return 0;
}
