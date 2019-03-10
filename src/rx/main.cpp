#include <SDL.h>

#include <rx/core/memory/system_allocator.h>

#include <rx/core/statics.h>
#include <rx/core/string.h>

#include <rx/console/console.h>
#include <rx/console/variable.h>

#include <rx/render/frontend.h>
#include <rx/render/buffer.h>
#include <rx/render/target.h>
#include <rx/render/texture.h>
#include <rx/render/backend_gl4.h>

#include <rx/input/input.h>

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

extern "C" {
  void __cxa_pure_virtual() {
    RX_ASSERT(false, "pure virtual function call");
  }
}

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

  const rx::math::vec3f vertices[]{
    {-1.0f, -1.0f, 0.0f },
    { 0.0f, -1.0f, 0.0f },
    { 1.0f, -1.0f, 0.0f },
    { 0.0f,  1.0f, 0.0f }
  };

  const rx_u8 elements[]{
    0,3,1,
    1,3,2,
    2,3,0,
    0,1,2
  };

  {
    rx::render::frontend::allocation_info allocation_info;
    rx::render::backend_gl4 gl4{allocation_info};

    rx::render::frontend renderer{&rx::memory::g_system_allocator, &gl4, allocation_info};

    // create, record and initialize buffer
    auto buffer{renderer.create_buffer(RX_RENDER_TAG("test"))};
    buffer->write_vertices(vertices, sizeof vertices, sizeof(rx::math::vec3f));
    buffer->write_elements(elements, sizeof elements);
    buffer->record_attribute(rx::render::buffer::attribute::category::k_f32, 3, 0);
    renderer.initialize_buffer(RX_RENDER_TAG("test"), buffer);

    // create, record and initialize color texture
    auto attach{renderer.create_texture2D(RX_RENDER_TAG("test"))};
    attach->record_format(rx::render::texture::data_format::k_rgba_u8);
    attach->record_wrap({
      rx::render::texture::wrap_options::category::k_clamp_to_edge,
      rx::render::texture::wrap_options::category::k_clamp_to_edge,
      rx::render::texture::wrap_options::category::k_clamp_to_edge});
    attach->record_filter({ true, false, false });
    attach->record_dimensions({1600, 900});
    renderer.initialize_texture(RX_RENDER_TAG("test"), attach);

    // create, record and initialize target
    auto target{renderer.create_target(RX_RENDER_TAG("test"))};
    target->request_depth(rx::render::texture::data_format::k_d24, {1600, 900});
    target->attach_texture(attach);
    renderer.initialize_target(RX_RENDER_TAG("test"), target);

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

#if 0
      renderer.draw_elements(
        RX_RENDER_TAG("test"),
        state,
        target,
        buffer,
        program,
        rx::render::primitive_type::k_triangles,
        0,
        12
      );
#endif

      if (renderer.process())
      {
        renderer.swap();
      }
    }

    renderer.destroy_target(RX_RENDER_TAG("test"), target);
    renderer.destroy_texture(RX_RENDER_TAG("test"), attach);
    renderer.destroy_buffer(RX_RENDER_TAG("test"), buffer);

    renderer.process();
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
