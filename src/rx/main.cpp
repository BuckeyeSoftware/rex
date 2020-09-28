#include <signal.h> // signal, SIG{INT,TERM,HUP,QUIT,KILL,PIPE,ALRM,STOP}

// #define SDL_MAIN_HANDLED
#include <SDL.h>

#include "rx/console/interface.h"
#include "rx/console/variable.h"

#include "rx/render/frontend/context.h"
#include "rx/render/backend/gl4.h"
#include "rx/render/backend/gl3.h"
#include "rx/render/backend/es3.h"
#include "rx/render/backend/null.h"

#include "rx/core/filesystem/file.h"

#include "rx/core/profiler.h"
#include "rx/core/global.h"
#include "rx/core/abort.h"
#include "rx/core/log.h"
#include "rx/core/ptr.h"

#include "rx/core/math/sin.h"

#include "rx/game.h"
#include "rx/display.h"

using namespace Rx;

RX_CONSOLE_V2IVAR(
  display_resolution,
  "display.resolution",
  "display resolution",
  Math::Vec2i(800, 600),
  Math::Vec2i(4096, 4096),
  Math::Vec2i(1600, 900));

RX_CONSOLE_IVAR(
  display_fullscreen,
  "display.fullscreen",
  "fullscreen mode (0 = windowed, 1 = windowed borderless, 2 = fullscreen)",
  0,
  2,
  0);

RX_CONSOLE_SVAR(
  display_name,
  "display.name",
  "name of display to run on",
  "");

RX_CONSOLE_BVAR(
  display_resizable,
  "display.resizable",
  "if the display can be resized",
  true);

RX_CONSOLE_BVAR(
  display_hdr,
  "display.hdr",
  "use HDR output if supported",
  false);

RX_CONSOLE_IVAR(
  display_swap_interval,
  "display.swap_interval",
  "swap interval (0 = immediate updates, 1 = updates syncronized with vertical retrace (vsync), -1 = adaptive vsync)",
  -1,
  1,
  -1);

RX_CONSOLE_SVAR(
  render_driver,
  "render.driver",
  "which driver to use for renderer (gl3, gl4, es3, null)",
  "gl4");

RX_CONSOLE_BVAR(
  profile_cpu,
  "profile.cpu",
  "collect cpu proflile samples",
  true);

RX_CONSOLE_BVAR(
  profile_gpu,
  "profile.gpu",
  "collect gpu profile samples",
  false);

RX_CONSOLE_BVAR(
  profile_local,
  "profile.local",
  "restrict profiling to localhost",
  true);

RX_CONSOLE_IVAR(
  profile_port,
  "profile.port",
  "port to run profiler on",
  1024,
  65536,
  0x4597);

RX_CONSOLE_IVAR(
  thread_pool_threads,
  "thread_pool.threads",
  "maximum number of threads for thread pool (0 uses the # of CPUs detected)",
  0,
  256,
  0);

RX_CONSOLE_IVAR(
  thread_pool_static_pool_size,
  "thread_pool.static_pool_size",
  "size of static pool for jobs before another static pool is allocated",
  32,
  4096,
  1024);

static Concurrency::Atomic<Game::Status> g_status{Game::Status::RESTART};

static Global<Filesystem::File> g_log{"system", "log", "log.log", "wb"};

RX_LOG("main", logger);

int main(int _argc, char** _argv) {
  extern Ptr<Game> create(Render::Frontend::Context&);

  (void)_argc;
  (void)_argv;

  auto catch_signal = [](int) {
    g_status.store(Game::Status::SHUTDOWN);
  };

  signal(SIGINT, catch_signal);
  signal(SIGTERM, catch_signal);

  // Don't catch these signals in debug.
#if !defined(RX_DEBUG)
  signal(SIGILL, catch_signal);
  signal(SIGABRT, catch_signal);
  signal(SIGFPE, catch_signal);
  signal(SIGSEGV, catch_signal);
#endif

#if !defined(RX_PLATFORM_WINDOWS)
  signal(SIGHUP, catch_signal);
  signal(SIGQUIT, catch_signal);
  signal(SIGKILL, catch_signal);
  signal(SIGPIPE, catch_signal);
  signal(SIGALRM, catch_signal);
  signal(SIGSTOP, catch_signal);
#endif

  // Link all globals into their respective groups.
  Globals::link();

  // Give the logger a stream to write to.
  // Filesystem::File log{"log.log", "wb"};
  (void)Log::subscribe(&g_log);

  if (!Console::Interface::load("config.cfg")) {
    Console::Interface::save("config.cfg");
  }

  const Size static_pool_size = *thread_pool_static_pool_size;
  const Size threads = *thread_pool_threads ? *thread_pool_threads : SDL_GetCPUCount();
  Globals::find("system")->find("thread_pool")->init(threads, static_pool_size);

  // The following scope exists because anything inside here needs to go out
  // of scope before the engine can safely return from main. This is because main
  // explicitly destroys globals that may be relied upon by objects in the scope
  // below, such as the logging event handles.
  {
    // Register an event handler for every log's "on_queue" method which will
    // recieve messages from the log and replicate it on the engine console. When
    // the handles go out of scope, the console will no longer recieve those
    // messages.
    Vector<Log::WriteEvent::Handle> logging_event_handles;
    Globals::find("loggers")->each([&](GlobalNode* _logger) {
      logging_event_handles.push_back(_logger->cast<Rx::Log>()->on_queue(
        [](Log::Level _level, const String& _message) {
          switch (_level) {
          case Log::Level::k_error:
            Console::Interface::print("^rerror: ^w%s", _message);
            break;
          case Log::Level::k_info:
            Console::Interface::print("^cinfo: ^w%s", _message);
            break;
          case Log::Level::k_verbose:
            // Don't write verbose messages to the console.
            // console::interface::print("^yverbose: ^w%s", _message);
            break;
          case Log::Level::k_warning:
            Console::Interface::print("^mwarning: ^w%s", _message);
            break;
          }
        }));
    });

    // Initialize the others in any order.
    Globals::init();

    // Bind some useful console commands early
    Console::Interface::add_command("reset", "s",
                                    [](const Vector<Console::Command::Argument>& _arguments) {
        if (auto* variable{Console::Interface::find_variable_by_name(_arguments[0].as_string)}) {
          variable->reset();
          return true;
        }
        return false;
      });

    Console::Interface::add_command("clear", "",
                                    [](const Vector<Console::Command::Argument>&) {
        Console::Interface::clear();
        return true;
      });

    Console::Interface::add_command("exit", "",
                                    [](const Vector<Console::Command::Argument>&) {
        g_status = Game::Status::SHUTDOWN;
        return true;
      });

    Console::Interface::add_command("quit", "",
                                    [&](const Vector<Console::Command::Argument>&) {
        g_status = Game::Status::SHUTDOWN;
        return true;
      });

    Console::Interface::add_command("restart", "",
                                    [&](const Vector<Console::Command::Argument>&) {
        g_status = Game::Status::RESTART;
        return true;
      });

#if 0
    // Replace SDL2s allocator with our system allocator so we can track it's
    // memory usage.
    SDL_SetMemoryFunctions(
      [](Size _size) -> void* {
        return Memory::SystemAllocator::instance().allocate(_size);
      },
      [](Size _size, Size _elements) -> void* {
        Byte* data = Memory::SystemAllocator::instance().allocate(_size, _elements);
        if (data) {
          memset(data, 0, _size * _elements);
          return data;
        }
        return nullptr;
      },
      [](void* _data, Size _size) -> void* {
        return Memory::SystemAllocator::instance().reallocate(_data, _size);
      },
      [](void* _data){
        Memory::SystemAllocator::instance().deallocate(_data);
      }
    );
#endif
    SDL_SetMainReady();

    // The initial status is always |k_restart| so the restart loop can be
    // entered here. There's three states that a game can return from it's slice,
    // k_running, k_restart and k_shutdown.
    //
    // This is where engine restart is handled.
    while (g_status == Game::Status::RESTART) {
      if (!Console::Interface::load("config.cfg")) {
        Console::Interface::save("config.cfg");
      }

      if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        abort("failed to initialize video");
      }

      // Fetch all the displays
      const auto& displays =
        Display::displays(Memory::SystemAllocator::instance());

      // Search for the given display in the display list.
      const auto found_display_index =
        displays.find_if([&](const Display& _display) {
          return _display.name() == *display_name;
        });

      const Display* found_display =
        found_display_index != -1_z ? &displays[found_display_index] : nullptr;

      if (!found_display) {
        // Use the first display if we could not match a display.
        found_display = &displays.first();
        // Save that selection.
        display_name->set(found_display->name());
      }

      // Fetch the index of display inside the display list.
      const Size display_index = found_display - &displays.first();

      const auto& driver_name{render_driver->get()};
      const bool is_gl = driver_name.begins_with("gl");
      const bool is_es = driver_name.begins_with("es");

      int flags{0};
      if (is_gl || is_es) {
        flags |= SDL_WINDOW_OPENGL;
      }
      if (*display_resizable) {
        flags |= SDL_WINDOW_RESIZABLE;
      }
      if (*display_fullscreen == 1) {
        flags |= SDL_WINDOW_FULLSCREEN;
      } else if (*display_fullscreen == 2) {
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
      }

      if (is_gl || is_es) {
        if (is_gl) {
          SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
          if (driver_name == "gl4") {
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
          } else if (driver_name == "gl3") {
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
          }
        } else if (is_es) {
          SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
          if (driver_name == "es3") {
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
          }
        }

        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
        SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
      }

      SDL_Window* window{nullptr};
      int bit_depth{0};
      for (const char* depth{&"\xa\x8"[*display_hdr ? 0 : 1]}; *depth; depth++) {
        bit_depth = static_cast<int>(*depth);
        if (is_gl || is_es) {
          SDL_GL_SetAttribute(SDL_GL_RED_SIZE, bit_depth);
          SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, bit_depth);
          SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, bit_depth);
        }

        window = SDL_CreateWindow(
          "rex",
          SDL_WINDOWPOS_CENTERED_DISPLAY(display_index),
          SDL_WINDOWPOS_CENTERED_DISPLAY(display_index),
          display_resolution->get().w,
          display_resolution->get().h,
          flags);

        if (window) {
          break;
        }
      }

      if (!window) {
        abort("failed to create window");
      }

      if (bit_depth != 10) {
        display_hdr->set(false);
      }

      SDL_RaiseWindow(window);
      SDL_StartTextInput();

      {
        Ptr<Render::Backend::Context> backend;

        auto& allocator = Memory::SystemAllocator::instance();
        if (driver_name == "gl4") {
          backend = make_ptr<Render::Backend::GL4>(allocator, allocator, reinterpret_cast<void*>(window));
        } else if (driver_name == "gl3") {
          backend = make_ptr<Render::Backend::GL3>(allocator, allocator, reinterpret_cast<void*>(window));
        } else if (driver_name == "es3") {
          backend = make_ptr<Render::Backend::ES3>(allocator, allocator, reinterpret_cast<void*>(window));
        } else if (driver_name == "null") {
          backend = make_ptr<Render::Backend::Null>(allocator, allocator, reinterpret_cast<void*>(window));
        } else {
          abort("invalid driver");
        }

        if (!backend->init()) {
          abort("failed to initialize rendering backend");
        }

        if (is_gl || is_es) {
          SDL_GL_SetSwapInterval(*display_swap_interval);
        }

        Render::Frontend::Context frontend{allocator, backend.get()};

        // Quickly get a black screen.
        frontend.process();
        frontend.swap();

        SDL_SetRelativeMouseMode(SDL_TRUE);

        Ptr<Game> g = create(frontend);

        auto on_fullscreen_change{display_fullscreen->on_change([&](Sint32 _value) {
          if (_value == 0) {
            SDL_SetWindowFullscreen(window, 0);
          } else if (_value == 1) {
            SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
          } else if (_value == 2) {
            SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
          }

          Math::Vec2i size;
          SDL_GetWindowSize(window, &size.w, &size.h);
          g->on_resize(size.cast<Size>());
        })};

        auto on_swap_interval_change{display_swap_interval->on_change([&](Sint32 _value) {
          if (is_gl || is_es) {
            SDL_GL_SetSwapInterval(_value);
          } else {
            // TODO? this should be part of the backend.
          }
        })};

        auto on_display_resolution_changed{display_resolution->on_change([&](const Math::Vec2i& _resolution) {
          g->on_resize(_resolution.cast<Size>());
          SDL_SetWindowSize(window, _resolution.w, _resolution.h);
        })};

        if (!g->on_init()) {
          abort("game initialization failed");
        }

        // At this point, the game is officially running.
        g_status = Game::Status::RUNNING;

        frontend.process();
        frontend.swap();

        Input::Context input;
        while (g_status == Game::Status::RUNNING) {
          for (SDL_Event event; SDL_PollEvent(&event);) {
            Input::Event ievent;
            switch (event.type) {
            case SDL_QUIT:
              g_status = Game::Status::SHUTDOWN;
              break;
            case SDL_KEYDOWN:
            case SDL_KEYUP:
              ievent.type = Input::Event::Type::k_keyboard;
              ievent.as_keyboard.down = event.type == SDL_KEYDOWN;
              ievent.as_keyboard.scan_code = event.key.keysym.scancode;
              ievent.as_keyboard.symbol = event.key.keysym.sym;
              input.handle_event(ievent);
              break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
              ievent.type = Input::Event::Type::k_mouse_button;
              ievent.as_mouse_button.down = event.type == SDL_MOUSEBUTTONDOWN;
              ievent.as_mouse_button.button = event.button.button;
              input.handle_event(ievent);
              break;
            case SDL_MOUSEMOTION:
              ievent.type = Input::Event::Type::k_mouse_motion;
              ievent.as_mouse_motion.value = {event.motion.x, event.motion.y, event.motion.xrel, event.motion.yrel};
              input.handle_event(ievent);
              break;
            case SDL_MOUSEWHEEL:
              ievent.type = Input::Event::Type::k_mouse_scroll;
              ievent.as_mouse_scroll.value = {event.wheel.x, event.wheel.y};
              input.handle_event(ievent);
              break;
            case SDL_WINDOWEVENT:
              switch (event.window.event) {
              case SDL_WINDOWEVENT_SIZE_CHANGED:
                display_resolution->set({event.window.data1, event.window.data2});
                break;
              case SDL_WINDOWEVENT_MOVED:
                {
                  // When the display moves, attempt to determine if it moved to a different display.
                  Math::Rectangle<Sint32> extents;
                  extents.dimensions = display_resolution->get();
                  const Math::Vec2i offset{event.window.data1, event.window.data2};
                  extents.offset = offset;
                  logger->info("Window %s moved to %s", extents.dimensions,
                    extents.offset);

                  displays.each_fwd([&](const Display& _display) {
                    if (_display.contains(extents)) {
                      // The window has moved to another display, update the name
                      display_name->set(_display.name());
                      logger->info("Display changed to \"%s\"", display_name->get());
                      return false;
                    }
                    return true;
                  });
                }
                break;
              }
              break;
            case SDL_TEXTINPUT:
              ievent.type = Input::Event::Type::k_text_input;
              strcpy(ievent.as_text_input.contents, event.text.text);
              input.handle_event(ievent);
              break;
            case SDL_CLIPBOARDUPDATE:
              if (char* text = SDL_GetClipboardText()) {
                ievent.type = Input::Event::Type::k_clipboard;
                Utility::construct<String>(&ievent.as_clipboard.contents, text);
                SDL_free(text);
                input.handle_event(ievent);
              }
              break;
            }
          }

          if (g_status != Game::Status::RUNNING) {
            break;
          }

          // Execute one slice of the game.
          const Game::Status status = g->on_update(input);

          if (g_status != Game::Status::RUNNING) {
            break;
          }

          g_status = status;

          // Update the input system.
          const int updated = input.update(frontend.timer().delta_time());
          if (updated & Input::Context::k_clipboard) {
            SDL_SetClipboardText(input.clipboard().data());
          }

          if (updated & Input::Context::k_mouse_capture) {
            SDL_SetRelativeMouseMode(input.is_mouse_captured() ? SDL_TRUE : SDL_FALSE);
          }

          // Render ....
          g->on_render();

          // Submit all rendering work.
          if (frontend.process()) {
            if (frontend.swap()) {
              // ?
            }
          }
        }
      }

      Console::Interface::save("config.cfg");

      SDL_DestroyWindow(window);
    }

  }

  SDL_Quit();

  return 0;
}
