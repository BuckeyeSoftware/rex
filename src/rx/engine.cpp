#include <SDL.h>

#include "rx/render/backend/gl3.h"
#include "rx/render/backend/gl4.h"
#include "rx/render/backend/es3.h"
#include "rx/render/backend/null.h"

#include "rx/render/frontend/context.h"

#include "rx/engine.h"
#include "rx/display.h"

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

static constexpr const char* CONFIG = "config.cfg";

Engine::Engine()
  : m_render_backend{nullptr}
  , m_render_frontend{nullptr}
  , m_status{Status::RUNNING}
{
}

Engine::~Engine() {
  // Force application to deinitialize now.
  m_application = nullptr;

  // Save the console configuration
  m_console.save(CONFIG);

  auto& allocator = Memory::SystemAllocator::instance();
  allocator.destroy<Render::Frontend::Context>(m_render_frontend);
  allocator.destroy<Render::Backend::Context>(m_render_backend);

  SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

bool Engine::init() {
  // These need to be initialized early for the console.
  Globals::find("console")->init();

  // Early initialization may need values loaded from the configuration file.
  if (!m_console.load(CONFIG)) {
    m_console.save(CONFIG);
  }

  const Size static_pool_size = *thread_pool_static_pool_size;
  const Size threads = *thread_pool_threads ? *thread_pool_threads : SDL_GetCPUCount();
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

  // Bind some useful console commands early.
  m_console.add_command("reset", "s", [](Console::Context& console_, const Vector<Console::Command::Argument>& _arguments) {
    if (auto* variable = console_.find_variable_by_name(_arguments[0].as_string)) {
      variable->reset();
      return true;
    }
    return false;
  });

  m_console.add_command("clear", "", [](Console::Context& console_, const Vector<Console::Command::Argument>&) {
    console_.clear();
    return true;
  });

  m_console.add_command("exit", "", [this](Console::Context&, const Vector<Console::Command::Argument>&) {
    m_status = Status::SHUTDOWN;
    return true;
  });

  m_console.add_command("quit", "", [this](Console::Context&, const Vector<Console::Command::Argument>&) {
    m_status = Status::SHUTDOWN;
    return true;
  });

  m_console.add_command("restart", "", [this](Console::Context&, const Vector<Console::Command::Argument>&) {
    m_status = Status::RESTART;
    return true;
  });

  // Try this as early as possible.
  if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
    return false;
  }

  // Fetch all the displays
  m_displays = Display::displays(Memory::SystemAllocator::instance());

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
      "Rex",
      SDL_WINDOWPOS_CENTERED_DISPLAY(display_index),
      SDL_WINDOWPOS_CENTERED_DISPLAY(display_index),
      display_resolution->get().w,
      display_resolution->get().h,
      flags);

    if (window) {
      break;
    }
  }

  // Get the actual created window dimensions and update the display resolution.
  Math::Vec2i size;
  SDL_GetWindowSize(window, &size.w, &size.h);
  display_resolution->set(size);

  // Notify the input system of the possible resize.
  m_input.on_resize(size.cast<Size>());

  if (!window) {
    return false;
    // abort("failed to create window");
  }

  SDL_RaiseWindow(window);
  SDL_StartTextInput();
  SDL_SetRelativeMouseMode(SDL_TRUE);

  if (bit_depth != 10) {
    display_hdr->set(false);
  }

  // Create the rendering backend context.
  auto& allocator = Memory::SystemAllocator::instance();
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
    // abort("invalid driver");
  }

  if (!m_render_backend || !m_render_backend->init()) {
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
    return false;
  }

  // Quickly get a blank window.
  m_render_frontend->process();
  m_render_frontend->swap();

  // Create the application instance.
  m_application = create(this);
  if (!m_application || !m_application->on_init()) {
    return false;
  }

  auto on_display_fullscreen_change = display_fullscreen->on_change([&](Sint32 _value) {
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

  auto on_display_resolution_change = display_resolution->on_change([&](const Math::Vec2i& _resolution) {
    m_application->on_resize(_resolution.cast<Size>());
    m_input.on_resize(_resolution.cast<Size>());
    SDL_SetWindowSize(window, _resolution.w, _resolution.h);
  });

  if (!on_display_fullscreen_change || !on_display_swap_interval_change || !on_display_resolution_change) {
    return false;
  }

  m_on_display_fullscreen_change = Utility::move(*on_display_fullscreen_change);
  m_on_display_swap_interval_change = Utility::move(*on_display_swap_interval_change);
  m_on_display_resolution_change = Utility::move(*on_display_resolution_change);

  return true;
}

Engine::Status Engine::run() {
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
      case SDL_WINDOWEVENT_SIZE_CHANGED:
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
          //logger->info("Window %s moved to %s", extents.dimensions,
          //  extents.offset);

          m_displays.each_fwd([&](const Display& _display) {
            if (_display.contains(extents)) {
              // The window has moved to another display, update the name
              display_name->set(_display.name());
              // logger->info("Display changed to \"%s\"", display_name->get());
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

    m_input.handle_event(input);
  }

  // This can be rate limited (e.g lock 60fps).
  if (!m_application->on_update()) {
    m_status = Status::SHUTDOWN;
  }

  // Update the input system.
  const int updated = m_input.on_update(m_render_frontend->timer().delta_time());
  if (updated & Input::Context::CLIPBOARD) {
    SDL_SetClipboardText(m_input.clipboard().data());
  }

  if (updated & Input::Context::MOUSE_CAPTURE) {
    SDL_SetRelativeMouseMode(m_input.active_layer().is_mouse_captured() ? SDL_TRUE : SDL_FALSE);
  }

  m_application->on_render();

  // Submit all rendering work.
  if (m_render_frontend->process()) {
    m_render_frontend->swap();
  }

  return m_status;
}

} // namespace Rx
