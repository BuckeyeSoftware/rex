#include <SDL.h>

#include "rx/render/backend/gl3.h"
#include "rx/render/backend/gl4.h"
#include "rx/render/backend/es3.h"
#include "rx/render/backend/null.h"

#include "rx/render/frontend/context.h"
#include "rx/render/frontend/timer.h"

#include "rx/texture/loader.h"

#include "rx/engine.h"
#include "rx/display.h"

#if defined(RX_PLATFORM_EMSCRIPTEN)
#include <emscripten.h>
#endif

extern Rx::Ptr<Rx::Application> create(Rx::Engine* _engine);

namespace Rx {

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

#if defined(RX_PLATFORM_EMSCRIPTEN)
RX_CONSOLE_SVAR(
  render_driver,
  "render.driver",
  "which driver to use for renderer (es3, null)",
  "es3");
#else
RX_CONSOLE_SVAR(
  render_driver,
  "render.driver",
  "which driver to use for renderer (gl3, gl4, es3, null)",
  "gl4");
#endif

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

RX_CONSOLE_IVAR(
  app_update_hz,
  "app.update_hz",
  "the rate at which the app is updated (independent from framerate) [restarts the engine]",
  30,
  360,
  60);

RX_CONSOLE_SVAR(
  app_name,
  "app.name",
  "the name of the application",
  "");

RX_CONSOLE_SVAR(
  app_icon,
  "app.icon",
  "path to the application icon",
  "base/icon.png");

static constexpr const char* CONFIG = "config.cfg";

RX_LOG("engine", logger);

Engine::Engine()
  : m_console{Memory::SystemAllocator::instance()}
  , m_input{Memory::SystemAllocator::instance()}
  , m_render_backend{nullptr}
  , m_render_frontend{nullptr}
  , m_logging_event_handles{Memory::SystemAllocator::instance()}
  , m_displays{Memory::SystemAllocator::instance()}
  , m_status{Status::RUNNING}
  , m_accumulator{0.0f}
{
}

Engine::~Engine() {
  // Force application to deinitialize now.
  m_application = nullptr;

  // Save the console configuration.
  RX_ASSERT(m_console.save(CONFIG), "failed to save config");

  auto& allocator = Memory::SystemAllocator::instance();
  allocator.destroy<Render::Frontend::Context>(m_render_frontend);
  allocator.destroy<Render::Backend::Context>(m_render_backend);

  SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

bool Engine::init() {
  // These need to be initialized early for the console.
  Globals::find("console")->init();

  // Early initialization may need values loaded from the configuration file.
  if (!m_console.load(CONFIG) && !m_console.save(CONFIG)) {
    return false;
  }

  auto& allocator = Memory::SystemAllocator::instance();

  const Size static_pool_size = *thread_pool_static_pool_size;

  // Determine how many threads the Emscripten pool was preallocated with.

#if defined(RX_PLATFORM_EMSCRIPTEN)
  // We subtract 1 from PTHREAD_POOL_SIZE since the logger needs a thread.
  const Size threads = emscripten_get_compiler_setting("PTHREAD_POOL_SIZE") - 1;
#else
  const Size threads = *thread_pool_threads ? *thread_pool_threads : SDL_GetCPUCount();
#endif

  Globals::find("system")->find("thread_pool")->init(threads, static_pool_size);

  // Setup all the loggers to emit to our console.
  Globals::find("loggers")->each([&](GlobalNode* _logger) {
    auto on_queue = _logger->cast<Rx::Log>()->on_queue(
      [&](Log::Level _level, const String& _message) {
        switch (_level) {
        case Log::Level::ERROR:
          m_console.print("^rerror: ^w%s", _message);
          break;
        case Log::Level::INFO:
          m_console.print("^cinfo: ^w%s", _message);
          break;
        case Log::Level::VERBOSE:
          // Don't write verbose messages to the console.
          // m_console.print("^yverbose: ^w%s", _message);
          break;
        case Log::Level::WARNING:
          m_console.print("^mwarning: ^w%s", _message);
          break;
        }
      }
    );

    if (on_queue) {
      m_logging_event_handles.push_back(Utility::move(*on_queue));
    }
  });

  // Initialize any other globals not already initialized.
  Globals::init();

  auto cmd_reset = Console::Command::Delegate::create(
    [](Console::Context& console_, const Vector<Console::Command::Argument>& _arguments) {
      if (auto* variable = console_.find_variable_by_name(_arguments[0].as_string)) {
        variable->reset();
        return true;
      }
      return false;
    }
  );

  auto cmd_clear = Console::Command::Delegate::create(
    [](Console::Context& console_, const Vector<Console::Command::Argument>&) {
      console_.clear();
      return true;
    }
  );

  auto cmd_exit = Console::Command::Delegate::create(
    [this](Console::Context&, const Vector<Console::Command::Argument>&) {
      m_status = Status::SHUTDOWN;
      return true;
    }
  );

  auto cmd_quit = Console::Command::Delegate::create(
    [this](Console::Context&, const Vector<Console::Command::Argument>&) {
      m_status = Status::SHUTDOWN;
      return true;
    }
  );

  auto cmd_restart = Console::Command::Delegate::create(
    [this](Console::Context&, const Vector<Console::Command::Argument>&) {
      m_status = Status::RESTART;
      return true;
    }
  );

  if (!cmd_reset || !cmd_clear || !cmd_exit || !cmd_quit || !cmd_restart) {
    return false;
  }

  // Bind some useful console commands early.
  if (!m_console.add_command("reset", "s", Utility::move(*cmd_reset))) return false;
  if (!m_console.add_command("clear", "", Utility::move(*cmd_clear))) return false;
  if (!m_console.add_command("exit", "", Utility::move(*cmd_exit))) return false;
  if (!m_console.add_command("quit", "", Utility::move(*cmd_quit))) return false;
  if (!m_console.add_command("restart", "", Utility::move(*cmd_restart))) return false;

  // Try this as early as possible.
  if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
    logger->error("Failed to initialize video: %s", SDL_GetError());
    return false;
  }

  // Fetch all the displays
  auto displays = Display::displays(Memory::SystemAllocator::instance());
  if (!displays) {
    return false;
  }

  m_displays = Utility::move(*displays);

  // Search for the given display in the display list.
  const auto found_display_index =
    m_displays.find_if([&](const Display& _display) {
      return _display.name() == *display_name;
    });

  const Display* found_display =
    found_display_index ? &m_displays[*found_display_index] : nullptr;

  if (!found_display) {
    // Use the first display if we could not match a display.
    found_display = &m_displays.first();
    // Save that selection.
    display_name->set(found_display->name());
  }

  // Fetch the index of display inside the display list.
  const Size display_index = found_display - &m_displays.first();

  const auto& driver_name = render_driver->get();
  const bool is_gl = driver_name.begins_with("gl");
  const bool is_es = driver_name.begins_with("es");

  Uint32 flags = 0;
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
#if defined(RX_PLATFORM_EMSCRIPTEN)
    // When building for Emscripten assume ES 3.0 which is WebGL 2.0
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    // NOTE(dweiler): The default framebuffer cannot be sRGB in WebGL.
#else
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
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
      }
    }
    SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);
#endif

    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  }

  SDL_Window* window = nullptr;
  int bit_depth = 0;
  for (const char* depth{&"\xa\x8"[*display_hdr ? 0 : 1]}; *depth; depth++) {
    bit_depth = static_cast<int>(*depth);
    if (is_gl || is_es) {
      SDL_GL_SetAttribute(SDL_GL_RED_SIZE, bit_depth);
      SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, bit_depth);
      SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, bit_depth);
    }

    window = SDL_CreateWindow(
      app_name->get().is_empty()
        ? "Rex"
        : String::format(Memory::SystemAllocator::instance(), "Rex: %s", app_name->get()).data(),
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
    logger->error("Failed to create window: %s", SDL_GetError());
    return false;
  }

  if (!app_icon->get().is_empty()) {
    Texture::Loader loader{Memory::SystemAllocator::instance()};
    if (loader.load(app_icon->get(), Texture::PixelFormat::RGBA_U8, {64, 64})) {
      auto surface = SDL_CreateRGBSurfaceFrom(
        loader.data().data(),
        Sint32(loader.dimensions().w),
        Sint32(loader.dimensions().h),
        loader.bits_per_pixel(),
        loader.dimensions().w * loader.bits_per_pixel() / 8,
        0xFF000000,
        0x00FF0000,
        0x0000FF00,
        0x000000FF);
      if (surface) {
        SDL_SetWindowIcon(window, surface);
        SDL_FreeSurface(surface);
      }
    }
  }

  // Get the actual created window dimensions and update the display resolution.
  Math::Vec2i size;
  SDL_GetWindowSize(window, &size.w, &size.h);
  display_resolution->set(size);

  // Notify the input system of the possible resize.
  m_input.on_resize(size.cast<Size>());

  SDL_RaiseWindow(window);
  SDL_StartTextInput();
  SDL_SetRelativeMouseMode(SDL_TRUE);

  if (bit_depth != 10) {
    display_hdr->set(false);
  }

  // Create the rendering backend context.
  logger->verbose("Initializing renderer ...");
  if (driver_name == "gl4") {
    m_render_backend = allocator.create<Render::Backend::GL4>(allocator, reinterpret_cast<void*>(window));
  } else if (driver_name == "gl3") {
    m_render_backend = allocator.create<Render::Backend::GL3>(allocator, reinterpret_cast<void*>(window));
  } else if (driver_name == "es3") {
    m_render_backend = allocator.create<Render::Backend::ES3>(allocator, reinterpret_cast<void*>(window));
  } else if (driver_name == "null") {
    m_render_backend = allocator.create<Render::Backend::Null>(allocator, reinterpret_cast<void*>(window));
  } else {
    return false;
  }

  if (!m_render_backend || !m_render_backend->init()) {
    logger->error("Failed to initialize renderer");
    return false;
  }

  if (is_gl || is_es) {
    SDL_GL_SetSwapInterval(*display_swap_interval);
  }

  // Create the rendering frontend context.
  m_render_frontend = allocator.create<Render::Frontend::Context>(
    allocator,
    m_render_backend,
    display_resolution->get().cast<Size>(),
    display_hdr->get());

  if (!m_render_frontend) {
    logger->error("Failed to initialize renderer");
    return false;
  }

  // This blocks the browser too long. Defer it.
#if !defined(RX_PLATFORM_EMSCRIPTEN)
  // Quickly get a blank window.
  m_render_frontend->process();
  m_render_frontend->swap();
#endif

  // Create the application instance.
  logger->verbose("Initializing application ...");
  m_application = create(this);
  if (!m_application || !m_application->on_init()) {
    logger->error("Failed to initialize application");
    return false;
  }

  auto on_display_fullscreen_change = display_fullscreen->on_change([=, this](Sint32 _value) {
    if (_value == 0) {
      SDL_SetWindowFullscreen(window, 0);
    } else if (_value == 1) {
      SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    } else if (_value == 2) {
      SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
    }

    Math::Vec2i size;
    SDL_GetWindowSize(window, &size.w, &size.h);
    m_application->on_resize(size.cast<Size>());
    m_input.on_resize(size.cast<Size>());
  });

  auto on_display_swap_interval_change = display_swap_interval->on_change([](Sint32 _value) {
    const bool is_gl = render_driver->get().begins_with("gl");
    const bool is_es = render_driver->get().begins_with("es");
    if (is_gl || is_es) {
      SDL_GL_SetSwapInterval(_value);
    }
  });

  auto on_display_resolution_change = display_resolution->on_change([=, this](const Math::Vec2i& _resolution) {
    m_application->on_resize(_resolution.cast<Size>());
    m_input.on_resize(_resolution.cast<Size>());
    SDL_SetWindowSize(window, _resolution.w, _resolution.h);
  });

  auto on_app_update_hz_change = app_update_hz->on_change([&](Sint32) {
    m_status = Status::RESTART;
  });

  if (!on_display_fullscreen_change || !on_display_swap_interval_change || !on_display_resolution_change || !on_app_update_hz_change) {
    return false;
  }

  m_on_display_fullscreen_change = Utility::move(*on_display_fullscreen_change);
  m_on_display_swap_interval_change = Utility::move(*on_display_swap_interval_change);
  m_on_display_resolution_change = Utility::move(*on_display_resolution_change);
  m_on_app_update_hz_change = Utility::move(*on_app_update_hz_change);

  return true;
}

Engine::Status Engine::integrate() {
  const auto update_rate = 1.0 / Float64(app_update_hz->get());

  // Process all events from SDL.
  for (SDL_Event event; SDL_PollEvent(&event);) {
    Input::Event input;
    switch (event.type) {
    case SDL_QUIT:
      // Quickly end the engine when we recieve the QUIT event.
      return Status::SHUTDOWN;
    case SDL_KEYDOWN:
    case SDL_KEYUP:
      input.type = Input::Event::Type::KEYBOARD;
      input.as_keyboard.down = event.type == SDL_KEYDOWN;
      input.as_keyboard.scan_code = event.key.keysym.scancode;
      input.as_keyboard.symbol = event.key.keysym.sym;
      break;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
      input.type = Input::Event::Type::MOUSE_BUTTON;
      input.as_mouse_button.down = event.type == SDL_MOUSEBUTTONDOWN;
      input.as_mouse_button.button = event.button.button;
      // Translate SDL's top-left coordinates to bottom-left coordinates.
      input.as_mouse_button.position = {
        event.button.x,
        display_resolution->get().h - event.button.y};
      break;
    case SDL_MOUSEMOTION:
      input.type = Input::Event::Type::MOUSE_MOTION;
      // Translate SDL's top-left coordinates to bottom-left coordinates.
      input.as_mouse_motion.value = {
        event.motion.x,
        display_resolution->get().h - event.motion.y,
        event.motion.xrel,
        event.motion.yrel};
      break;
    case SDL_MOUSEWHEEL:
      input.type = Input::Event::Type::MOUSE_SCROLL;
      input.as_mouse_scroll.value = {event.wheel.x, event.wheel.y};
      break;
    case SDL_WINDOWEVENT:
      switch (event.window.event) {
      case SDL_WINDOWEVENT_RESIZED:
        {
          const Math::Vec2i size{event.window.data1, event.window.data2};
          display_resolution->set(size, false);
          m_application->on_resize(size.cast<Size>());
          m_input.on_resize(size.cast<Size>());
        }
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

          m_displays.each_fwd([&](const Display& _display) {
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
      input.type = Input::Event::Type::TEXT_INPUT;
      strcpy(input.as_text_input.contents, event.text.text);
      break;
    case SDL_CLIPBOARDUPDATE:
      if (char* text = SDL_GetClipboardText()) {
        input.type = Input::Event::Type::CLIPBOARD;
        Utility::construct<String>(&input.as_clipboard.contents, text);
        SDL_free(text);
      }
      break;
    }

    if (!m_input.handle_event(input)) {
      return Status::SHUTDOWN;
    }
  }

  if (!m_application->on_update(update_rate)) {
    return Status::SHUTDOWN;
  }

  // Update the input system.
  const int updated = m_input.on_update(update_rate);
  if (updated & Input::Context::CLIPBOARD) {
    SDL_SetClipboardText(m_input.clipboard().data());
  }

  if (updated & Input::Context::MOUSE_CAPTURE) {
    SDL_SetRelativeMouseMode(m_input.active_layer().is_mouse_captured() ? SDL_TRUE : SDL_FALSE);
  }

  return Status::RUNNING;
}

Engine::Status Engine::run() {
  const auto update_rate = 1.0 / Float64(app_update_hz->get());
  m_accumulator += m_render_frontend->timer().delta_time();
  while (m_accumulator >= update_rate) {
    auto status = integrate();
    if (status != Status::RUNNING) {
      m_status = status;
    }
    m_accumulator -= update_rate;
  }

  m_application->on_render();

  // Submit all rendering work.
  if (m_render_frontend->process()) {
    m_render_frontend->swap();
  }

  return m_status;
}

} // namespace Rx
