#include <SDL.h> // TODO remove

#include "rx/game.h"

#include "rx/render/frontend/interface.h"
#include "rx/render/frontend/timer.h"
#include "rx/render/frontend/target.h"

#include "rx/render/immediate2D.h"
#include "rx/render/immediate3D.h"
#include "rx/render/gbuffer.h"
#include "rx/render/skybox.h"
#include "rx/render/model.h"

#include "rx/math/camera.h"

#include "rx/console/interface.h"

using namespace rx;

static void frame_stats(const render::frontend::interface &_frontend, render::immediate2D &_immediate) {
  const render::frontend::frame_timer &_timer{_frontend.timer()};
  const math::vec2i &screen_size{_frontend.swapchain()->dimensions().cast<rx_s32>()};
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

  const math::vec2i &screen_size{_frontend.swapchain()->dimensions().cast<rx_s32>()};

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

static void memory_stats(const render::frontend::interface& _frontend, render::immediate2D& _immediate) {
  const auto stats{memory::g_system_allocator->stats()};
  const math::vec2i &screen_size{_frontend.swapchain()->dimensions().cast<rx_s32>()};

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

struct test_game
  : game
{
  test_game(render::frontend::interface& _frontend)
    : m_frontend{_frontend}
    , m_gbuffer{&m_frontend}
    , m_immediate2D{&m_frontend}
    , m_immediate3D{&m_frontend}
    , m_skybox{&m_frontend}
    , m_model{&m_frontend}
  {
    m_camera.translate = {-1.0f, 60.0f, -1000.0f};
  }

  virtual bool on_init() {
    m_gbuffer.create(m_frontend.swapchain()->dimensions());
    m_skybox.load("base/skyboxes/miramar/miramar.json5");
    m_model.load("base/models/elemental/elemental.json5");
    return true;
  }

  virtual bool on_slice(const input::input& _input) {
    static auto display_resolution{console::interface::get_from_name("display.resolution")->cast<math::vec2i>()};
    static auto display_swap_interval{console::interface::get_from_name("display.swap_interval")->cast<rx_s32>()};
    static auto display_fullscreen{console::interface::get_from_name("display.fullscreen")->cast<rx_s32>()};

    const auto& dimensions{display_resolution->get().cast<rx_f32>()};
    m_camera.projection = math::mat4x4f::perspective(90.0f, {0.1f, 2048.0f},
      dimensions.w / dimensions.h);

    rx_f32 move_speed{10.0f};
    const rx_f32 sens{0.2f};
    const auto &delta{_input.mouse().movement()};
    math::vec3f move{static_cast<rx_f32>(delta.y) * sens, static_cast<rx_f32>(delta.x) * sens, 0.0f};
    m_camera.rotate = m_camera.rotate + move;

    if (_input.keyboard().is_held(SDL_SCANCODE_LSHIFT, true)) {
      move_speed = 200.0f;
    } else {
      move_speed = 10.0f;
    }

    if (_input.keyboard().is_held(SDL_SCANCODE_W, true)) {
      const auto f{m_camera.to_mat4().z};
      m_camera.translate += math::vec3f(f.x, f.y, f.z) * (move_speed * m_frontend.timer().delta_time());
    }
    if (_input.keyboard().is_held(SDL_SCANCODE_S, true)) {
      const auto f{m_camera.to_mat4().z};
      m_camera.translate -= math::vec3f(f.x, f.y, f.z) * (move_speed * m_frontend.timer().delta_time());
    }
    if (_input.keyboard().is_held(SDL_SCANCODE_D, true)) {
      const auto l{m_camera.to_mat4().x};
      m_camera.translate += math::vec3f(l.x, l.y, l.z) * (move_speed * m_frontend.timer().delta_time());
    }
    if (_input.keyboard().is_held(SDL_SCANCODE_A, true)) {
      const auto l{m_camera.to_mat4().x};
      m_camera.translate -= math::vec3f(l.x, l.y, l.z) * (move_speed * m_frontend.timer().delta_time());
    }

    if (_input.keyboard().is_released(SDL_SCANCODE_F1, true)) {
      switch (display_swap_interval->get()) {
      case -1:
        display_swap_interval->set(0);
        break;
      case 0:
        display_swap_interval->set(1);
        break;
      case 1:
        display_swap_interval->set(-1);
        break;
      }
    }

    if (_input.keyboard().is_released(SDL_SCANCODE_ESCAPE, true)) {
      return false;
    }

    if (_input.keyboard().is_released(SDL_SCANCODE_F12, true)) {
      display_fullscreen->set((display_fullscreen->get() + 1) % 3);
    }

    math::transform model_xform;
    model_xform.rotate = {0.0f, 90.0f, 0.0f};

    render::frontend::state state;
    state.viewport.record_dimensions(display_resolution->get().cast<rx_size>());

    m_frontend.clear(
      RX_RENDER_TAG("gbuffer"),
      state,
      m_gbuffer.target(),
      RX_RENDER_CLEAR_DEPTH,
      {1.0f, 0.0f, 0.0f, 0.0f});

    m_frontend.clear(
      RX_RENDER_TAG("gbuffer"),
      state,
      m_gbuffer.target(),
      RX_RENDER_CLEAR_COLOR(0),
      {0.0f, 0.0f, 0.0f, 0.0f});

    m_model.render(m_gbuffer.target(), model_xform.to_mat4(), m_camera.view(), m_camera.projection);
    m_skybox.render(m_gbuffer.target(), m_camera.view(), m_camera.projection);

    m_immediate3D.render(m_gbuffer.target(), m_camera.view(), m_camera.projection);

    state.viewport.record_dimensions(display_resolution->get().cast<rx_size>());

    m_frontend.blit(
      RX_RENDER_TAG("test"),
      state,
      m_gbuffer.target(),
      0,
      m_frontend.swapchain(),
      0);

    frame_stats(m_frontend, m_immediate2D);
    render_stats(m_frontend, m_immediate2D);
    memory_stats(m_frontend, m_immediate2D);

    m_immediate2D.render(m_frontend.swapchain());

    return true;
  }

  void on_resize(const math::vec2z& _dimensions) {
    m_gbuffer.resize(_dimensions);
    m_frontend.resize(_dimensions);
  }

  render::frontend::interface& m_frontend;

  render::gbuffer m_gbuffer;
  render::immediate2D m_immediate2D;
  render::immediate3D m_immediate3D;
  render::skybox m_skybox;
  render::model m_model;

  math::camera m_camera;
};

game* create(render::frontend::interface& _frontend) {
  return memory::g_system_allocator->create<test_game>(_frontend);
}
