#include <SDL.h>

#include <rx/core/memory/system_allocator.h>

#include <rx/core/statics.h>
#include <rx/core/string.h>

#include <rx/console/console.h>
#include <rx/console/variable.h>

#include <rx/input/input.h>

#include <rx/render/frontend.h>
#include <rx/render/target.h>
#include <rx/render/gbuffer.h>
#include <rx/render/buffer.h>
#include <rx/render/technique.h>
#include <rx/render/backend_gl4.h>

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

#include <rx/model/iqm.h>
#include <rx/math/quat.h>

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
    rx::render::backend_gl4 backend{&rx::memory::g_system_allocator, reinterpret_cast<void*>(window)};
    rx::render::frontend frontend{&rx::memory::g_system_allocator, &backend};

    rx::render::target* target{frontend.create_target(RX_RENDER_TAG("default"))};
    target->request_swapchain();

    rx::render::gbuffer gbuffer{&frontend};
    gbuffer.create({1600, 900});

    rx::render::buffer* triangle{frontend.create_buffer(RX_RENDER_TAG("triangle"))};
    triangle->record_stride(sizeof(rx::math::vec3f));
    triangle->record_element_type(rx::render::buffer::element_category::k_u32);
    triangle->record_type(rx::render::buffer::category::k_static);
    triangle->record_attribute(rx::render::buffer::attribute::category::k_f32, 3, 0);

    rx::model::iqm iqm{&rx::memory::g_system_allocator};
    if (iqm.load("test.iqm")) {
      triangle->write_vertices(iqm.positions().data(), iqm.positions().size() * sizeof(rx::math::vec3f));
      triangle->write_elements(iqm.elements().data(), iqm.elements().size() * sizeof(rx_u32));
    }

    frontend.initialize_buffer(RX_RENDER_TAG("triangle"), triangle);

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

      // clear gbuffer albedo, normal & emission for testing
      frontend.clear(RX_RENDER_TAG("gbuffer albedo"),
        gbuffer, RX_RENDER_CLEAR_COLOR(0), {1.0f, 0.0f, 0.0f, 1.0f});

      frontend.clear(RX_RENDER_TAG("gbuffer normal"),
        gbuffer, RX_RENDER_CLEAR_COLOR(1), {0.0f, 1.0f, 0.0f, 1.0f});

      frontend.clear(RX_RENDER_TAG("gbuffer emission"),
        gbuffer, RX_RENDER_CLEAR_COLOR(2), {0.0f, 0.0f, 1.0f, 1.0f});

      rx::render::technique* program{frontend.find_technique_by_name("gbuffer-test")};
      frontend.draw_elements(
        RX_RENDER_TAG("test"),
        {},
        gbuffer,
        triangle,
        *program,
        iqm.elements().size(),
        0,
        rx::render::primitive_type::k_triangles,
        "");

      frontend.clear(RX_RENDER_TAG("default"),
        target, RX_RENDER_CLEAR_COLOR(0), {1.0f, 0.0f, 0.0f, 1.0f});

      if (frontend.process()) {
        if (frontend.swap()) {
          const auto stats{rx::memory::g_system_allocator->stats()};
          char format[1024];
          snprintf(format, sizeof format, "%d fps | %.2f mspf | mem a/%zu, r/r:%zu a:%zu, d/%zu, u/r:%s a:%s, p/r:%s a:%s",
            frontend.timer().fps(),
            frontend.timer().mspf(),
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

    frontend.destroy_target(RX_RENDER_TAG("default"), target);
    frontend.destroy_buffer(RX_RENDER_TAG("test"), triangle);
  }

  rx::console::console::save("config.cfg");

  SDL_GL_DeleteContext(context);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}

int main(int argc, char **argv) {
  // trigger allocator initialization first
  auto malloc_allocator{rx::static_globals::find("malloc_allocator")};
  auto system_allocator{rx::static_globals::find("system_allocator")};
  RX_ASSERT(malloc_allocator, "malloc allocator missing");
  RX_ASSERT(system_allocator, "system allocator missing");
  malloc_allocator->init();
  system_allocator->init();

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

  // finalize allocators
  system_allocator->fini();
  malloc_allocator->fini();

  return result;
}