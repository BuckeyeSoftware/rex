#include <SDL.h>

#include "rx/console/interface.h"
#include "rx/console/variable.h"

#include "rx/render/frontend/interface.h"
#include "rx/render/frontend/technique.h"
#include "rx/render/frontend/material.h"
#include "rx/render/frontend/target.h"
#include "rx/render/frontend/buffer.h"

#include "rx/render/backend/gl4.h"
#include "rx/render/backend/gl3.h"

#include "rx/render/immediate2D.h"
#include "rx/render/immediate3D.h"
#include "rx/render/gbuffer.h"
#include "rx/render/skybox.h"
#include "rx/render/model.h"

#include "rx/math/camera.h"
#include "rx/math/transform.h"

#include "rx/input/input.h"

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

struct quad_vertex {
  math::vec2f position;
  math::vec2f coordinate;
};

static constexpr const quad_vertex k_quad_vertices[]{
  {{-1.0f, 1.0f}, {0.0f, 1.0f}},
  {{1.0f, 1.0f}, {1.0f, 1.0f}},
  {{-1.0f, -1.0f}, {0.0f, 0.0f}},
  {{1.0f, -1.0f}, {1.0f, 0.0f}}
};

static constexpr const rx_byte k_quad_elements[]{
  0, 1, 2, 3
};

static void frame_stats(const render::frontend::interface &_frontend, render::immediate2D &_immediate) {
  const render::frontend::frame_timer &_timer{_frontend.timer()};
  const math::vec2i &screen_size{*display_resolution};
  const math::vec2i box_size{600, 200};
  const rx_s32 box_bottom{25};
  const rx_s32 box_middle{box_bottom + (box_size.h / 2)};
  const rx_s32 box_top{box_bottom + box_size.h};
  const rx_s32 box_left{screen_size.w / 2 - box_size.w / 2};
  const rx_s32 box_center{box_left + box_size.w / 2};
  const rx_s32 box_right{box_left + box_size.w};

  _immediate.frame_queue().record_rectangle({box_left, box_bottom}, box_size, 0, {0.0f, 0.0f, 0.0f, 0.5f});

  const auto k_frame_scale{16.667 * 2.0f};
  vector<math::vec2i> points;
  _timer.frame_times().each_fwd([&](const render::frontend::frame_timer::frame_time &_time) {
    const auto delta_x{(_timer.ticks() * _timer.resolution() - _time.life) / render::frontend::frame_timer::k_frame_history_seconds};
    const auto delta_y{algorithm::min(_time.frame / k_frame_scale, 1.0)};
    const math::vec2i point{box_right - rx_s32(delta_x * box_size.w), box_top - rx_s32(delta_y * box_size.h)};
    points.push_back(point);
  });

  const rx_size n_points{points.size()};
  for (rx_size i{1}; i < n_points; i++) {
    _immediate.frame_queue().record_line(points[i - 1], points[i], 0, 1, {0.0f, 1.0f, 0.0f, 1.0f});
  }

  _immediate.frame_queue().record_line({box_left, box_bottom}, {box_left, box_top}, 0, 1, {1.0f, 1.0f, 1.0f, 1.0f});
  _immediate.frame_queue().record_line({box_center, box_bottom}, {box_center, box_top}, 0, 1, {1.0f, 1.0f, 1.0f, 1.0f});
  _immediate.frame_queue().record_line({box_right, box_bottom}, {box_right, box_top}, 0, 1, {1.0f, 1.0f, 1.0f, 1.0f});
  _immediate.frame_queue().record_line({box_left, box_bottom}, {box_right, box_bottom}, 0, 1, {1.0f, 1.0f, 1.0f, 1.0f});
  _immediate.frame_queue().record_line({box_left, box_middle}, {box_right, box_middle}, 0, 1, {1.0f, 1.0f, 1.0f, 1.0f});
  _immediate.frame_queue().record_line({box_left, box_top}, {box_right, box_top}, 0, 1, {1.0f, 1.0f, 1.0f, 1.0f});
  _immediate.frame_queue().record_text("Inconsolata-Regular", {box_center, box_top + 5}, 18, 1.0f, render::immediate2D::text_align::k_center, "Frame Time", {1.0f, 1.0f, 1.0f, 1.0f});
  _immediate.frame_queue().record_text("Inconsolata-Regular", {box_right + 5, box_top - 5}, 18, 1.0f, render::immediate2D::text_align::k_left, "0.0", {1.0f, 1.0f, 1.0f, 1.0f});
  _immediate.frame_queue().record_text("Inconsolata-Regular", {box_right + 5, box_middle - 5}, 18, 1.0f, render::immediate2D::text_align::k_left, string::format("%.1f", k_frame_scale * .5), {1.0f, 1.0f, 1.0f, 1.0f});
  _immediate.frame_queue().record_text("Inconsolata-Regular", {box_right + 5, box_bottom - 5}, 18, 1.0f, render::immediate2D::text_align::k_left, string::format("%.1f", k_frame_scale), {1.0f, 1.0f, 1.0f, 1.0f});
}

static void render_stats(const render::frontend::interface &_frontend, render::immediate2D &_immediate) {
  const auto &buffer_stats{_frontend.stats(render::frontend::resource::type::k_buffer)};
  const auto &program_stats{_frontend.stats(render::frontend::resource::type::k_program)};
  const auto &target_stats{_frontend.stats(render::frontend::resource::type::k_target)};
  const auto &texture1D_stats{_frontend.stats(render::frontend::resource::type::k_texture1D)};
  const auto &texture2D_stats{_frontend.stats(render::frontend::resource::type::k_texture2D)};
  const auto &texture3D_stats{_frontend.stats(render::frontend::resource::type::k_texture3D)};
  const auto &textureCM_stats{_frontend.stats(render::frontend::resource::type::k_textureCM)};

  math::vec2i offset{25, 25};

  auto color_ratio{[](rx_size _used, rx_size _total) -> rx_u32 {
    const math::vec3f bad{1.0f, 0.0f, 0.0f};
    const math::vec3f good{0.0f, 1.0f, 0.0f};
    const rx_f32 scaled{static_cast<rx_f32>(_used) / static_cast<rx_f32>(_total)};
    const math::vec3f color{bad * scaled + good * (1.0f - scaled)};
    return (rx_u32(color.r * 255.0f) << 24) |
           (rx_u32(color.g * 255.0f) << 16) |
           (rx_u32(color.b * 255.0f) << 8) |
           0xFF;
  }};

  auto render_stat{[&](const char *_label, const auto &_stats) {
    const auto format{string::format("^w%s: ^[%x]%zu ^wof ^m%zu ^g%s",
                                         _label, color_ratio(_stats.used, _stats.total), _stats.used, _stats.total,
                                         string::human_size_format(_stats.memory))};

    _immediate.frame_queue().record_text("Consolas-Regular", offset, 20, 1.0f,
                                         render::immediate2D::text_align::k_left, format, {1.0f, 1.0f, 1.0f, 1.0f});
    offset.y += 20;
  }};

  const auto &command_buffer{_frontend.get_command_buffer()};
  const rx_size commands_used{command_buffer.used()};
  const rx_size commands_total{command_buffer.size()};
  _immediate.frame_queue().record_text("Consolas-Regular", offset, 20, 1.0f,
                                       render::immediate2D::text_align::k_left,
                                       string::format("commands: ^[%x]%s ^wof ^g%s",
                                                      color_ratio(commands_used, commands_total),
                                                      string::human_size_format(commands_used),
                                                      string::human_size_format(commands_total)),
                                       {1.0f, 1.0f, 1.0f, 1.0f});

  offset.y += 20;

  render_stat("texturesCM", textureCM_stats);
  render_stat("textures3D", texture3D_stats);
  render_stat("textures2D", texture2D_stats);
  render_stat("textures1D", texture1D_stats);
  render_stat("programs", program_stats);
  render_stat("buffers", buffer_stats);
  render_stat("targets", target_stats);

  auto render_number{[&](const char* _name, rx_size _number) {
    _immediate.frame_queue().record_text(
      "Consolas-Regular",
      offset,
      20,
      1.0f,
      render::immediate2D::text_align::k_left,
      string::format("%s: %zu", _name, _number),
      {1.0f, 1.0f, 1.0f, 1.0f});
    offset.y += 20;
  }};

  render_number("points", _frontend.points());
  render_number("lines", _frontend.lines());
  render_number("triangles", _frontend.triangles());
  render_number("vertices", _frontend.vertices());
  render_number("blits", _frontend.blit_calls());
  render_number("clears", _frontend.clear_calls());
  render_number("draws", _frontend.draw_calls());
 
  const math::vec2i &screen_size{*display_resolution};
  const auto& info{_frontend.get_device_info()};

  _immediate.frame_queue().record_text(
    "Consolas-Regular",
    {25, screen_size.h - 25},
    16,
    1.0f,
    render::immediate2D::text_align::k_left,
    string::format("%s via %s on %s by %s", renderer_driver->get(), info.version, info.renderer, info.vendor),
    {1.0f, 1.0f, 1.0f, 1.0f});

  // mspf and fps
  const auto &_timer{_frontend.timer()};
  _immediate.frame_queue().record_text(
    "Consolas-Regular",
    screen_size - math::vec2i{25, 25},
    16,
    1.0f,
    render::immediate2D::text_align::k_right,
    string::format("MSPF: %.2f | FPS: %d", _timer.mspf(), _timer.fps()),
    {1.0f, 1.0f, 1.0f, 1.0f});
}

static void memory_stats(const render::frontend::interface&, render::immediate2D& _immediate) {
  const auto stats{memory::g_system_allocator->stats()};
  const math::vec2i &screen_size{*display_resolution};

  int y = 25;
  auto line{[&](const string &_line) {
    _immediate.frame_queue().record_text(
      "Consolas-Regular",
      math::vec2i{screen_size.x - 25, y},
      16,
      1.0f,
      render::immediate2D::text_align::k_right,
      _line,
      {1.0f, 1.0f, 1.0f, 1.0f});
    y += 16;
  }};

  line(string::format("used memory (requested): %s", string::human_size_format(stats.used_request_bytes)));
  line(string::format("used memory (actual):    %s", string::human_size_format(stats.used_actual_bytes)));
  line(string::format("peak memory (requested): %s", string::human_size_format(stats.peak_request_bytes)));
  line(string::format("peak memory (actual):    %s", string::human_size_format(stats.peak_actual_bytes)));
}

int entry(int argc, char **argv) {
  (void)argc;
  (void)argv;

  if (!console::interface::load("config.cfg")) {
    // immediately save the default options on load failure
    console::interface::save("config.cfg");
  }

  // enumerate displays to find the one with the given name
  SDL_Init(SDL_INIT_VIDEO);

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
  if (renderer_driver->get() == "gl4") {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
  } else {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  }

  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
  SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  SDL_Window* window{nullptr};
  int bit_depth{0};
  for (const char* depth{"\xa\x8" + (*display_hdr ? 0 : 1)}; *depth; depth++) {
    bit_depth = static_cast<int>(*depth);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, bit_depth);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, bit_depth);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, bit_depth);

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

  SDL_GLContext context{SDL_GL_CreateContext(window)};

  SDL_GL_SetSwapInterval(*display_swap_interval);
  SDL_SetRelativeMouseMode(SDL_TRUE);

  math::camera camera{math::mat4x4f::perspective(90.0f, {0.1f, 2048.0f},
    display_resolution->get().cast<rx_f32>().w / display_resolution->get().cast<rx_f32>().h)};
  camera.translate = {0.0f, 0.0f, -10.0f};

  render::backend::interface* backend{nullptr};
  if (renderer_driver->get() == "gl4") {
    backend = memory::g_system_allocator->create<render::backend::gl4>(
        &memory::g_system_allocator, reinterpret_cast<void*>(window));
  } else if (renderer_driver->get() == "gl3") {
    backend = memory::g_system_allocator->create<render::backend::gl3>(
        &memory::g_system_allocator, reinterpret_cast<void*>(window));
  }

  {
    render::frontend::interface frontend{&memory::g_system_allocator, backend};
    frontend.process();
    frontend.swap();

    render::gbuffer gbuffer{&frontend};
    gbuffer.create(display_resolution->get().cast<rx_size>());

    render::immediate2D immediate2D{&frontend};
    render::immediate3D immediate3D{&frontend};
    render::skybox skybox{&frontend};
    render::model model{&frontend};

    skybox.load("base/skyboxes/miramar/miramar.json5");
    model.load("base/models/elemental/elemental.json5");

    input::input input;
    while (!input.keyboard().is_released(SDLK_ESCAPE, false))
    {
      rx_f32 move_speed{10.0f};
      const rx_f32 sens{0.2f};
      const auto &delta{input.mouse().movement()};
      math::vec3f move{static_cast<rx_f32>(delta.y) * sens, static_cast<rx_f32>(delta.x) * sens, 0.0f};
      camera.rotate = camera.rotate + move;

      if (input.keyboard().is_held(SDL_SCANCODE_LSHIFT, true)) {
        move_speed = 200.0f;
      } else {
        move_speed = 10.0f;
      }

      if (input.keyboard().is_held(SDL_SCANCODE_W, true)) {
        const auto f{camera.to_mat4().z};
        camera.translate += math::vec3f(f.x, f.y, f.z) * (move_speed * frontend.timer().delta_time());
      }
      if (input.keyboard().is_held(SDL_SCANCODE_S, true)) {
        const auto f{camera.to_mat4().z};
        camera.translate -= math::vec3f(f.x, f.y, f.z) * (move_speed * frontend.timer().delta_time());
      }
      if (input.keyboard().is_held(SDL_SCANCODE_D, true)) {
        const auto l{camera.to_mat4().x};
        camera.translate += math::vec3f(l.x, l.y, l.z) * (move_speed * frontend.timer().delta_time());
      }
      if (input.keyboard().is_held(SDL_SCANCODE_A, true)) {
        const auto l{camera.to_mat4().x};
        camera.translate -= math::vec3f(l.x, l.y, l.z) * (move_speed * frontend.timer().delta_time());
      }

      if (input.keyboard().is_released(SDL_SCANCODE_F3, true)) {
        display_swap_interval->set(display_swap_interval->get() == 0 ? 1 : 0);
        SDL_GL_SetSwapInterval(display_swap_interval->get());
      }

      frontend.clear(
        RX_RENDER_TAG("gbuffer"),
        {},
        gbuffer.target(),
        RX_RENDER_CLEAR_DEPTH,
        {1.0f, 0.0f, 0.0f, 0.0f});

      frontend.clear(
        RX_RENDER_TAG("gbuffer"),
        {},
        gbuffer.target(),
        RX_RENDER_CLEAR_COLOR(0),
        {0.0f, 0.0f, 0.0f, 0.0f});

      math::mat4x4f modelm{math::mat4x4f::scale({0.2f, 0.2f, 0.2f})
        * math::mat4x4f::rotate({0.0f, 90.0f, 0.0f})
        * math::mat4x4f::translate({0.0f, 0.0f, 0.0f})};

      skybox.render(gbuffer.target(), camera.view(), camera.projection());
      model.render(gbuffer.target(),  modelm, camera.view(), camera.projection());
      immediate3D.render(gbuffer.target(), camera.view(), camera.projection());

      frontend.blit(
        RX_RENDER_TAG("test"),
        {},
        gbuffer.target(),
        0,
        frontend.swapchain(),
        0);

      frame_stats(frontend, immediate2D);
      render_stats(frontend, immediate2D);
      memory_stats(frontend, immediate2D);

      immediate2D.render(frontend.swapchain());

      if (frontend.process()) {
        if (frontend.swap()) {
        }
      }

      input.update(frontend.timer().delta_time());

      // translate SDL events
      for (SDL_Event event; SDL_PollEvent(&event);)
      {
        input::event ievent;
        switch (event.type)
        {
        case SDL_KEYDOWN:
        case SDL_KEYUP:
          ievent.type = input::event_type::k_keyboard;
          ievent.as_keyboard.down = event.type == SDL_KEYDOWN;
          ievent.as_keyboard.scan_code = event.key.keysym.scancode;
          ievent.as_keyboard.symbol = event.key.keysym.sym;
          input.handle_event(utility::move(ievent));
          break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
          ievent.type = input::event_type::k_mouse_button;
          ievent.as_mouse_button.down = event.type == SDL_MOUSEBUTTONDOWN;
          ievent.as_mouse_button.button = event.button.button;
          input.handle_event(utility::move(ievent));
          break;
        case SDL_MOUSEMOTION:
          ievent.type = input::event_type::k_mouse_motion;
          ievent.as_mouse_motion.value = {event.motion.x, event.motion.y, event.motion.xrel, event.motion.yrel};
          input.handle_event(utility::move(ievent));
          break;
        case SDL_MOUSEWHEEL:
          ievent.type = input::event_type::k_mouse_scroll;
          ievent.as_mouse_scroll.value = {event.wheel.x, event.wheel.y};
          input.handle_event(utility::move(ievent));
          break;
        }
      }
    }
  }

  memory::g_system_allocator->destroy<render::backend::interface>(backend);

  console::interface::save("config.cfg");

  SDL_GL_DeleteContext(context);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}

int main(int argc, char **argv) {
  // trigger allocator initialization first
  auto system_allocator{static_globals::find("system_allocator")};
  RX_ASSERT(system_allocator, "system allocator missing");
  system_allocator->init();

  // trigger log initialization
  auto log{static_globals::find("log")};
  RX_ASSERT(log, "log missing");

  log->init();

  // initialize all other static globals
  static_globals::init();

  // run the engine
  const auto result{entry(argc, argv)};

  // finalize all other static globals
  static_globals::fini();

  // finalize log
  log->fini();

  // finalize allocator
  system_allocator->fini();

  return result;
}