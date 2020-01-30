#include <signal.h>

#include <SDL.h>

#include "rx/console/interface.h"
#include "rx/console/variable.h"

#include "rx/render/frontend/interface.h"
#include "rx/render/backend/gl4.h"
#include "rx/render/backend/gl3.h"
#include "rx/render/backend/vk.h"
#include "rx/render/backend/es3.h"
#include "rx/render/backend/null.h"

#include "rx/core/profiler.h"
#include "rx/core/global.h"
#include "rx/core/abort.h"
#include "rx/core/log.h"

#include "rx/core/math/sin.h"

#include "rx/game.h"
#include "rx/display.h"

#include "lib/remotery.h"

using namespace rx;

RX_CONSOLE_V2IVAR(
  display_resolution,
  "display.resolution",
  "display resolution",
  math::vec2i(800, 600),
  math::vec2i(4096, 4096),
  math::vec2i(1600, 900));

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
  "which driver to use for renderer (gl3, gl4, es3, vk, null)",
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

static concurrency::atomic<game::status> g_status{game::status::k_restart};

RX_LOG("main", logger);

int main(int _argc, char** _argv) {
  (void)_argc;
  (void)_argv;

  auto catch_signal{[](int) {
    g_status.store(game::status::k_shutdown);
  }};

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
  globals::link();

  global_group* system_group{globals::find("system")};
  global_group* console_group{globals::find("console")};

  // Explicitly initialize globals that need to be initialized in a specific
  // order for things to work.
  system_group->find("allocator")->init();
  system_group->find("logger")->init();
  system_group->find("profiler")->init();
  system_group->find("thread_pool")->init(SDL_GetCPUCount());

  console_group->init();

  // The following scope exists because anything inside here needs to go out
  // of scope before the engine can safely return from main. This is because main
  // explicitly destroys globals that may be relied upon by objects in the scope
  // below, such as the logging event handles.
  {
    // Register an event handler for every log's "on_write" method which will
    // recieve messages from the log and replicate it on the engine console. When
    // the handles go out of scope, the console will no longer recieve those
    // messages.
    vector<log::event_type::handle> logging_event_handles;
    globals::find("loggers")->each([&](global_node* _logger) {
      logging_event_handles.push_back(_logger->cast<rx::log>()->on_write(
        [](log::level _level, const string& _message) {
          switch (_level) {
          case log::level::k_error:
            console::interface::print("^rerror: ^w%s", _message);
            break;
          case log::level::k_info:
            console::interface::print("^cinfo: ^w%s", _message);
            break;
          case log::level::k_verbose:
            // Don't write verbose messages to the console.
            break;
          case log::level::k_warning:
            console::interface::print("^mwarning: ^w%s", _message);
            break;
          }
        }));
    });

    // Initialize the others in any order.
    globals::init();

    // Bind some useful console commands early
    console::interface::add_command("reset", "s",
      [](const vector<console::command::argument>& _arguments) {
        if (auto* variable{console::interface::find_variable_by_name(_arguments[0].as_string)}) {
          variable->reset();
          return true;
        }
        return false;
      });

    console::interface::add_command("clear", "",
      [](const vector<console::command::argument>&) {
        console::interface::clear();
        return true;
      });

    console::interface::add_command("exit", "",
      [](const vector<console::command::argument>&) {
        g_status = game::status::k_shutdown;
        return true;
      });

    console::interface::add_command("quit", "",
      [&](const vector<console::command::argument>&) {
        g_status = game::status::k_shutdown;
        return true;
      });

    console::interface::add_command("restart", "",
      [&](const vector<console::command::argument>&) {
        g_status = game::status::k_restart;
        return true;
      });

    // Replace SDL2s allocator with our system allocator so we can track it's
    // memory usage.
    SDL_SetMemoryFunctions(
      [](rx_size _size) -> void* {
        return memory::g_system_allocator->allocate(_size);
      },
      [](rx_size _size, rx_size _elements) -> void* {
        rx_byte* data{memory::g_system_allocator->allocate(_size * _elements)};
        if (data) {
          memset(data, 0, _size * _elements);
          return data;
        }
        return nullptr;
      },
      [](void* _data, rx_size _size) -> void* {
        return memory::g_system_allocator->reallocate(reinterpret_cast<rx_byte*>(_data), _size);
      },
      [](void* _data){
        memory::g_system_allocator->deallocate(reinterpret_cast<rx_byte*>(_data));
      }
    );
    SDL_SetMainReady();

    // The initial status is always |k_restart| so the restart loop can be
    // entered here. There's three states that a game can return from it's slice,
    // k_running, k_restart and k_shutdown.
    //
    // This is where engine restart is handled.
    while (g_status == game::status::k_restart) {
      if (!console::interface::load("config.cfg")) {
        console::interface::save("config.cfg");
      }

      if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        abort("failed to initialize video");
      }


      // Fetch all the displays
      const auto& displays{display::displays(&memory::g_system_allocator)};

      // Search for the given display in the display list.
      const display* found_display{nullptr};
      displays.each_fwd([&](const display& _display) {
        if (_display.name() == *display_name) {
          found_display = &_display;
          return false;
        }
        return true;
      });

      if (!found_display) {
        // Use the first display if we could not match a display.
        found_display = &displays.first();
        // Save that selection.
        display_name->set(found_display->name());
      }

      // Fetch the index of display inside the display list.
      const rx_size display_index = found_display - &displays.first();

      const auto& driver_name{render_driver->get()};
      const bool is_gl{driver_name.begins_with("gl")};
      const bool is_es{driver_name.begins_with("es")};

      int flags{0};
      if (is_gl || is_es) {
        flags |= SDL_WINDOW_OPENGL;
      }
      if(driver_name == "vk") {
          flags |= SDL_WINDOW_VULKAN;
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

      render::backend::interface* backend{nullptr};
      if (driver_name == "gl4") {
        backend = memory::g_system_allocator->create<render::backend::gl4>(
          &memory::g_system_allocator, reinterpret_cast<void*>(window));
      } else if (driver_name == "gl3") {
        backend = memory::g_system_allocator->create<render::backend::gl3>(
          &memory::g_system_allocator, reinterpret_cast<void*>(window));
      } else if (driver_name == "es3") {
        backend = memory::g_system_allocator->create<render::backend::es3>(
          &memory::g_system_allocator, reinterpret_cast<void*>(window));
      } else if(driver_name == "vk") {
        backend = memory::g_system_allocator->create<render::backend::vk>(
          &memory::g_system_allocator, reinterpret_cast<void*>(window));
      } else if (driver_name == "null") {
        backend = memory::g_system_allocator->create<render::backend::null>(
          &memory::g_system_allocator, reinterpret_cast<void*>(window));
      } else {
        abort("invalid driver");
        
      }

      if (!backend->init()) {
        abort("failed to initialize rendering backend");
      }

      SDL_GL_SetSwapInterval(*display_swap_interval);

      {
        render::frontend::interface frontend{&memory::g_system_allocator, backend};

        rmtSettings* settings{rmt_Settings()};

        settings->reuse_open_port = RMT_TRUE;
        settings->maxNbMessagesPerUpdate = 128;
        settings->port = *profile_port;
        settings->limit_connections_to_localhost = *profile_local;

        settings->malloc = [](void*, rx_u32 _bytes) -> void* {
          return memory::g_system_allocator->allocate(_bytes);
        };

        settings->realloc = [](void*, void* _data, rx_u32 _bytes) ->void* {
          return memory::g_system_allocator->reallocate(reinterpret_cast<rx_byte*>(_data), _bytes);
        };

        settings->free = [](void*, void* _data) {
          memory::g_system_allocator->deallocate(reinterpret_cast<rx_byte*>(_data));
        };

        Remotery* remotery{nullptr};
        if (rmt_CreateGlobalInstance(&remotery) == RMT_ERROR_NONE) {
          auto set_thread_name{[](void*, const char* _name) {
            rmt_SetCurrentThreadName(_name);
          }};

          if (*profile_cpu) {
            profiler::instance().bind_cpu({
              reinterpret_cast<void*>(remotery),
              set_thread_name,
              [](void*, const char* _tag) {
                rmt_BeginCPUSampleDynamic(_tag, RMTSF_Aggregate);
              },
              [](void*) {
                rmt_EndCPUSample();
              }
            });
          }

          if (*profile_gpu) {
            if (is_gl || is_es) {
              rmt_BindOpenGL();
            }

            profiler::instance().bind_gpu({
              reinterpret_cast<void*>(&frontend),
              set_thread_name,
              [](void* _context, const char* _tag) {
                reinterpret_cast<render::frontend::interface*>(_context)->profile(_tag);
              },
              [](void* _context) {
                reinterpret_cast<render::frontend::interface*>(_context)->profile(nullptr);
              }
            });
          }
        }

        // Quickly get a black screen.
        frontend.process();
        frontend.swap();

        SDL_SetRelativeMouseMode(SDL_TRUE);

        // Create the game.
        extern game* create(render::frontend::interface&);
        game* g = create(frontend);

        auto on_fullscreen_change{display_fullscreen->on_change([&](rx_s32 _value) {
          if (_value == 0) {
            SDL_SetWindowFullscreen(window, 0);
          } else if (_value == 1) {
            SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
          } else if (_value == 2) {
            SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
          }

          math::vec2i size;
          SDL_GetWindowSize(window, &size.w, &size.h);
          g->on_resize(size.cast<rx_size>());
        })};

        auto on_swap_interval_change{display_swap_interval->on_change([&](rx_s32 _value) {
          if (is_gl || is_es) {
            SDL_GL_SetSwapInterval(_value);
          } else {
            // TODO? this should be part of the backend.
          }
        })};

        auto on_display_resolution_changed{display_resolution->on_change([&](const math::vec2i& _resolution) {
          g->on_resize(_resolution.cast<rx_size>());
          SDL_SetWindowSize(window, _resolution.w, _resolution.h);
        })};

        if (!g->on_init()) {
          memory::g_system_allocator->destroy<game>(g);
          abort("game initialization failed");
        }

        // At this point, the game is officially running.
        g_status = game::status::k_running;

        frontend.process();
        frontend.swap();

        input::input input;
        while (g_status == game::status::k_running) {
          for (SDL_Event event; SDL_PollEvent(&event);) {
            input::event ievent;
            switch (event.type) {
            case SDL_QUIT:
              g_status = game::status::k_shutdown;
              break;
            case SDL_KEYDOWN:
            case SDL_KEYUP:
              ievent.type = input::event_type::k_keyboard;
              ievent.as_keyboard.down = event.type == SDL_KEYDOWN;
              ievent.as_keyboard.scan_code = event.key.keysym.scancode;
              ievent.as_keyboard.symbol = event.key.keysym.sym;
              input.handle_event(ievent);
              break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
              ievent.type = input::event_type::k_mouse_button;
              ievent.as_mouse_button.down = event.type == SDL_MOUSEBUTTONDOWN;
              ievent.as_mouse_button.button = event.button.button;
              input.handle_event(ievent);
              break;
            case SDL_MOUSEMOTION:
              ievent.type = input::event_type::k_mouse_motion;
              ievent.as_mouse_motion.value = {event.motion.x, event.motion.y, event.motion.xrel, event.motion.yrel};
              input.handle_event(ievent);
              break;
            case SDL_MOUSEWHEEL:
              ievent.type = input::event_type::k_mouse_scroll;
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
                  math::rectangle<rx_s32> extents;
                  extents.dimensions = display_resolution->get();
                  const math::vec2i offset{event.window.data1, event.window.data2};
                  extents.offset = offset;
                  logger(log::level::k_info, "Window %s moved to %s",
                    extents.dimensions, extents.offset);

                  displays.each_fwd([&](const display& _display) {
                    if (_display.contains(extents)) {
                      // The window has moved to another display, update the name
                      display_name->set(_display.name());
                      logger(log::level::k_info, "Display changed to \"%s\"", display_name->get());
                      return false;
                    }
                    return true;
                  });
                }
                break;
              }
              break;
            case SDL_TEXTINPUT:
              ievent.type = input::event_type::k_text_input;
              strcpy(ievent.as_text_input.contents, event.text.text);
              input.handle_event(ievent);
              break;
            case SDL_CLIPBOARDUPDATE:
              // input.update_clipboard();
              {
                char* text{SDL_GetClipboardText()};
                if (text) {
                  ievent.type = input::event_type::k_clipboard;
                  utility::construct<string>(&ievent.as_clipboard.contents, text);
                  SDL_free(text);
                  input.handle_event(ievent);
                }
              }
              break;
            }
          }

          if (g_status != game::status::k_running) {
            break;
          }

          // Execute one slice of the game.
          const game::status status{g->on_slice(input)};

          if (g_status != game::status::k_running) {
            break;
          }

          g_status = status;

          // Update the input system.
          const int updated{input.update(frontend.timer().delta_time())};
          if (updated & input::input::k_clipboard) {
            SDL_SetClipboardText(input.clipboard().data());
          }

          if (updated & input::input::k_mouse_capture) {
            SDL_SetRelativeMouseMode(input.is_mouse_captured() ? SDL_TRUE : SDL_FALSE);
          }

          // Submit all rendering work.
          if (frontend.process()) {
            if (frontend.swap()) {
              // ?
            }
          }
        }

        memory::g_system_allocator->destroy<game>(g);

        if (remotery) {
          profiler::instance().unbind_cpu();
          profiler::instance().unbind_gpu();

          rmt_UnbindOpenGL();
          rmt_DestroyGlobalInstance(remotery);
        }
      }

      memory::g_system_allocator->destroy<render::backend::interface>(backend);

      console::interface::save("config.cfg");

      SDL_DestroyWindow(window);
    }

  }

  SDL_Quit();

  globals::fini();

  console_group->fini();

  system_group->find("thread_pool")->fini();
  system_group->find("profiler")->fini();
  system_group->find("logger")->fini();
  system_group->find("allocator")->fini();

  return 0;
}
