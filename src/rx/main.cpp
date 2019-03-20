#include <SDL.h>

#include <rx/core/memory/system_allocator.h>

#include <rx/core/statics.h>
#include <rx/core/string.h>

#include <rx/console/console.h>
#include <rx/console/variable.h>

#include <rx/input/input.h>

#include <rx/render/renderer.h>

RX_CONSOLE_V2IVAR(
  display_resolution,
  "display.resolution",
  "display resolution",
  rx::math::vec2i(800, 600),
  rx::math::vec2i(4096, 4096),
  rx::math::vec2i(1600, 900));

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

// #include <rx/render/immediate.h>

#if 0
static void draw_frame_graph(rx::render::renderer& _renderer) {
  auto& _queue = _renderer.immediates();
  const auto& _time = _renderer.time();
  const auto& timestamps = _time.timestamps();

  static constexpr const rx_f32 k_scale{16.66*2.0f};

  const rx::math::vec2i box_size{600, 200};
  const rx_s32 box_bottom{25};
  const rx_s32 box_middle{box_bottom + (box_size.h / 2)};
  const rx_s32 box_top{box_bottom + box_size.h};
  const rx_s32 box_left{1600/2 - box_size.w/2};
  const rx_s32 box_center{box_left+(box_size.w/2)};
  const rx_s32 box_right{box_left+box_size.w};

  _queue.record_rectangle({box_left, box_bottom}, box_size, 0, {0.0f, 0.0f, 0.0f, 1.0f});

  rx::array<rx::math::vec2i> points;
  timestamps.each_fwd([&](const rx::render::frame_timer::timestamp& _timestamp) {
    const rx_f32 delta_x{static_cast<rx_f32>((_time.life_time() - _timestamp.life_time) / rx::render::frame_timer::k_timestamp_seconds)};
    const rx_f32 delta_y{static_cast<rx_f32>(rx::min(_timestamp.frame_time / k_scale, 1.0))};
    const rx::math::vec2i point{
      box_right - static_cast<rx_s32>(delta_x * box_size.w),
      box_top - static_cast<rx_s32>(delta_y * box_size.h)
    };
    points.push_back(point);
  });

  _queue.record_scissor({box_left, box_bottom}, box_size);
  for (rx_size i{1}; i < points.size(); i++) {
    _queue.record_line(points[i - 1], points[i], 0, 1, {0.0f, 1.0f, 0.0f, 1.0f});
  }
  _queue.record_scissor({-1, -1}, {-1, -1});

  _queue.record_line({box_left, box_bottom}, {box_left, box_top}, 0, 1, {1.0f, 1.0f, 1.0f, 1.0f});
  _queue.record_line({box_center, box_bottom}, {box_center, box_top}, 0, 1, {1.0f, 1.0f, 1.0f, 1.0f});
  _queue.record_line({box_right, box_bottom}, {box_right, box_top}, 0, 1, {1.0f, 1.0f, 1.0f, 1.0f});
  _queue.record_line({box_left, box_bottom}, {box_right, box_bottom}, 0, 1, {1.0f, 1.0f, 1.0f, 1.0f});
  _queue.record_line({box_left, box_middle}, {box_right, box_middle}, 0, 1, {1.0f, 1.0f, 1.0f, 1.0f});
  _queue.record_line({box_left, box_top}, {box_right, box_top}, 0, 1, {1.0f, 1.0f, 1.0f, 1.0f});
}

#endif

int entry(int argc, char **argv) {
  (void)argc;
  (void)argv;

  if (!rx::console::console::load("config.cfg")) {
    // immediately save the default options on load failure
    rx::console::console::save("config.cfg");
  }

  // enumerate displays to find the one with the given name
  SDL_Init(SDL_INIT_VIDEO);

  const rx::string& want_name{display_name->get()};
  int display_index{0};
  int displays{SDL_GetNumVideoDisplays()};
  for (int i{0}; i < displays; i++) {
    const char* name{SDL_GetDisplayName(i)};
    if (name && want_name == name) {
      display_index = i;
      break;
    }
  }

  const char* name{SDL_GetDisplayName(display_index)};
  display_name->set(name ? name : "");

  int flags{SDL_WINDOW_OPENGL};

  if (*display_resizable) {
    flags |= SDL_WINDOW_RESIZABLE;
  }

  if (*display_fullscreen == 1) {
    flags |= SDL_WINDOW_FULLSCREEN;
  } else if (*display_fullscreen == 2) {
    flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);

  const auto bit_depth{*display_hdr ? 10 : 8};
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, bit_depth);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, bit_depth);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, bit_depth);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
  SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  SDL_Window* window =
    SDL_CreateWindow(
      "rex",
      SDL_WINDOWPOS_CENTERED_DISPLAY(display_index),
      SDL_WINDOWPOS_CENTERED_DISPLAY(display_index),
      display_resolution->get().x,
      display_resolution->get().y,
      flags);

  if (!window) {
    abort();
  }

  SDL_GLContext context =
    SDL_GL_CreateContext(window);

  SDL_GL_SetSwapInterval(1);

  {
    rx::render::renderer renderer{&rx::memory::g_system_allocator, "gl4", reinterpret_cast<void*>(window)};
    rx::input::input input;

    while (!input.keyboard().is_released(SDLK_ESCAPE, false)) {
      input.update(0);

      // translate SDL events
      for (SDL_Event event; SDL_PollEvent(&event); ) {
        rx::input::event ievent;
        switch (event.type) {
        case SDL_KEYDOWN:
        case SDL_KEYUP:
          ievent.type = rx::input::event_type::k_keyboard;
          ievent.as_keyboard.down = event.type == SDL_KEYDOWN;
          ievent.as_keyboard.scan_code = event.key.keysym.scancode;
          ievent.as_keyboard.symbol = event.key.keysym.sym;
          input.handle_event(rx::utility::move(ievent));
          break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
          ievent.type = rx::input::event_type::k_mouse_button;
          ievent.as_mouse_button.down = event.type == SDL_MOUSEBUTTONDOWN;
          ievent.as_mouse_button.button = event.button.button;
          input.handle_event(rx::utility::move(ievent));
          break;
        case SDL_MOUSEMOTION:
          ievent.type = rx::input::event_type::k_mouse_motion;
          ievent.as_mouse_motion.value = { event.motion.x, event.motion.y };
          input.handle_event(rx::utility::move(ievent));
          break;
        case SDL_MOUSEWHEEL:
          ievent.type = rx::input::event_type::k_mouse_scroll;
          ievent.as_mouse_scroll.value = { event.wheel.x, event.wheel.y };
          input.handle_event(rx::utility::move(ievent));
          break;
        }
      }

      const auto& delta{input.mouse().movement()};
      rx::math::vec3f move{static_cast<rx_f32>(delta.y), static_cast<rx_f32>(delta.x), 0.0f};
      //camera.rotate = camera.rotate + move;

      if (input.keyboard().is_held(SDL_SCANCODE_W, true)) {
        //const auto f{camera.to_mat4().z};
        //camera.translate = camera.translate + rx::math::vec3f(f.x, f.y, f.z) * (10.0f * timer.delta_time());
      }
      if (input.keyboard().is_held(SDL_SCANCODE_S, true)) {
        //const auto f{camera.to_mat4().z};
        //camera.translate = camera.translate - rx::math::vec3f(f.x, f.y, f.z) * (10.0f * timer.delta_time());
      }
      if (input.keyboard().is_held(SDL_SCANCODE_D, true)) {
        //const auto l{camera.to_mat4().x};
        //camera.translate = camera.translate + rx::math::vec3f(l.x, l.y, l.z) * (10.0f * timer.delta_time());
      }
      if (input.keyboard().is_held(SDL_SCANCODE_A, true)) {
        //const auto l{camera.to_mat4().x};
        //camera.translate = camera.translate - rx::math::vec3f(l.x, l.y, l.z) * (10.0f * timer.delta_time());
      }

      if (renderer.update()) {
        const auto stats{rx::memory::g_system_allocator->stats()};
        char format[1024];
        snprintf(format, sizeof format, "%d fps | %.2f mspf | mem a/%zu, r/r:%zu a:%zu, d/%zu, u/r:%s a:%s, p/r:%s a:%s",
          renderer.timer().fps(),
          renderer.timer().mspf(),
          stats.allocations,
          stats.request_reallocations,
          stats.actual_reallocations,
          stats.deallocations,
          rx::string::human_size_format(stats.used_request_bytes).data(),
          rx::string::human_size_format(stats.used_actual_bytes).data(),
          rx::string::human_size_format(stats.peak_request_bytes).data(),
          rx::string::human_size_format(stats.peak_actual_bytes).data());

        SDL_SetWindowTitle(window, format);
      }
    }
  }

  rx::console::console::save("config.cfg");

  SDL_GL_DeleteContext(context);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}


int main(int argc, char **argv) {
  // trigger system allocator initialization first
  auto system_alloc{rx::static_globals::find("system_allocator")};
  RX_ASSERT(system_alloc, "system allocator missing");

  // initialize system allocator and remove from static globals list
  system_alloc->init();

  // trigger log initialization
  auto log{rx::static_globals::find("log")};
  RX_ASSERT(log, "log missing");

  log->init();

  // initialize all other static globals
  rx::static_globals::init();

  // run the engine
  const auto result{entry(argc, argv)};

  // finalize all other static globals
  rx::static_globals::fini();

  // finalize log
  log->fini();

  // finalize system allocator
  system_alloc->fini();

  return result;
}