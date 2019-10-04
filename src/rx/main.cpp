#include <SDL.h>
#include <signal.h>

#include "rx/console/interface.h"
#include "rx/console/variable.h"

#include "rx/render/frontend/interface.h"
#include "rx/render/backend/gl4.h"
#include "rx/render/backend/gl3.h"

#include "rx/core/profiler.h"
#include "rx/core/global.h"
#include "rx/core/abort.h"

#include "rx/game.h"

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
  renderer_driver,
  "renderer.driver",
  "which driver to use for renderer (gl3, gl4, null)",
  "gl4");

static concurrency::atomic<game::status> g_status{game::status::k_restart};

int main(int _argc, char** _argv) {
  (void)_argc;
  (void)_argv;

  auto catch_signal{[](int) {
    g_status.store(game::status::k_shutdown);
  }};

  signal(SIGINT, catch_signal);
  signal(SIGILL, catch_signal);
  signal(SIGABRT, catch_signal);
  signal(SIGFPE, catch_signal);
  signal(SIGSEGV, catch_signal);
  signal(SIGTERM, catch_signal);
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

  // Explicitly initialize globals that need to be initialized in a specific
  // order for things to work.
  globals::find("system")->find("allocator")->init();
  globals::find("system")->find("logger")->init();
  globals::find("system")->find("thread_pool")->init(SDL_GetCPUCount());
  globals::find("system")->find("profiler")->init();

  // Initialize the others in any order.
  globals::init();

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

    const string &want_name{display_name->get()};
    int display_index{0};
    int displays{SDL_GetNumVideoDisplays()};
    for (int i{0}; i < displays; i++) {
      const char *name{SDL_GetDisplayName(i)};
      if (name && want_name == name) {
        display_index = i;
        break;
      }
    }

    const char *name{SDL_GetDisplayName(display_index)};
    display_name->set(name ? name : "");

    const bool is_opengl{renderer_driver->get().begins_with("gl")};
    int flags{0};
    if (is_opengl) {
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

    if (is_opengl) {
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
      if (renderer_driver->get() == "gl4") {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
      } else if (renderer_driver->get() == "gl3") {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
      }

      SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
      SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
      SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
      SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    }

    SDL_Window* window{nullptr};
    int bit_depth{0};
    for (const char* depth{"\xa\x8" + (*display_hdr ? 0 : 1)}; *depth; depth++) {
      bit_depth = static_cast<int>(*depth);
      if (is_opengl) {
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

    //SDL_GL_SetSwapInterval(*display_swap_interval);

    render::backend::interface* backend{nullptr};
    if (renderer_driver->get() == "gl4") {
      backend = memory::g_system_allocator->create<render::backend::gl4>(
          &memory::g_system_allocator, reinterpret_cast<void*>(window));
    } else if (renderer_driver->get() == "gl3") {
      backend = memory::g_system_allocator->create<render::backend::gl3>(
          &memory::g_system_allocator, reinterpret_cast<void*>(window));
    }

    if (!backend->init()) {
      abort("failed to initialize rendering backend");
    }

    {
      render::frontend::interface frontend{&memory::g_system_allocator, backend};

      Remotery* remotery{nullptr};
      if (rmt_CreateGlobalInstance(&remotery) == RMT_ERROR_NONE) {
        auto set_thread_name{[](void*, const char* _name) {
          rmt_SetCurrentThreadName(_name);
        }};

        rmt_BindOpenGL();

        profiler::instance().bind_cpu({
          reinterpret_cast<void*>(remotery),
          set_thread_name,
          [](void*, const char* _tag) {
            rmt_BeginCPUSampleDynamic(_tag, 0);
          },
          [](void*) {
            rmt_EndCPUSample();
          }
        });

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

      // Quickly get a black screen.
      frontend.process();
      frontend.swap();

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
        if (is_opengl) {
          SDL_GL_SetSwapInterval(_value);
        } else {
          // TODO? this should be part of the backend.
        }
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
      while (g_status.load() == game::status::k_running) {
        for (SDL_Event event; SDL_PollEvent(&event);) {
          input::event ievent;
          switch (event.type) {
          case SDL_QUIT:
            g_status = game::status::k_shutdown;
            break;
          case SDL_KEYDOWN:
          case SDL_KEYUP:
            // Simple engine restart.
            if (event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_GRAVE) {
              printf("restarting\n");
              g_status = game::status::k_restart;
              break;
            }

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
              g->on_resize(display_resolution->get().cast<rx_size>());
              break;
            }
          }
        }

        if (g_status != game::status::k_running) {
          break;
        }

        // Execute one slice of the game.
        g_status = g->on_slice(input);

        // Update the input system.
        input.update(frontend.timer().delta_time());

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

  SDL_Quit();

  globals::fini();

  globals::find("system")->find("thread_pool")->fini();
  globals::find("system")->find("profiler")->fini();
  globals::find("system")->find("logger")->fini();
  globals::find("system")->find("allocator")->fini();

  return 0;
}
