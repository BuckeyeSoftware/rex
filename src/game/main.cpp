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
#include "rx/math/noise/perlin.h"

#include "rx/core/filesystem/file.h"

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

#if 0
static void write_noise_map(const vector<rx_f32>& _noise_map, const math::vec2i& _dimensions) {
  filesystem::file out{"test.ppm", "w"};
  out.print("P3\n");
  out.print("%zu %zu\n", _dimensions.w, _dimensions.h);
  out.print("255\n");

  for (rx_s32 y{0}; y < _dimensions.h; y++) {
    for (rx_s32 x{0}; x < _dimensions.w; x++) {
      const auto value{static_cast<rx_s32>(_noise_map[_dimensions.w * y + x] * 255.0f)};
      out.print("% 3d", value);
      out.print("% 3d", value);
      out.print("% 3d", value);
    }
    out.print("\n");
  }
}

static void write_color_map(const vector<math::vec3f>& _color_map, const math::vec2i& _dimensions) {
  filesystem::file out{"test.ppm", "w"};
  out.print("P3\n");
  out.print("%d %d\n", _dimensions.w, _dimensions.h);
  out.print("255\n");
  for (rx_s32 y{0}; y < _dimensions.h; y++) {
    for (rx_s32 x{0}; x < _dimensions.w; x++) {
      const auto value{(_color_map[_dimensions.w * y + x] * math::vec3f(255.0f, 255.0f, 255.0f)).cast<rx_s32>()};
      out.print("% 3d", value.r);
      out.print("% 3d", value.g);
      out.print("% 3d", value.b);
    }
    out.print("\n");
  }
}

static rx_f32 perlin_noise(rx_f32 _x, rx_f32 _y) {
  // return 0.0f;
  math::noise::perlin a{100};
  return a.noise(_x, _y);
}

rx_f32 inverse_lerp(rx_f32 _min, rx_f32 _max, rx_f32 _value) {
  if (math::abs(_max - _min) < 0.0001f) {
    return _min;
  }
  return (_value - _min) / (_max - _min);
}

struct terrain_type {
  math::vec3f color;
  rx_f32 height;
};

static vector<rx_f32> generate_noise_map(const math::vec2i& _dimensions, rx_f32 _scale, int _octaves, rx_f32 _persistance, rx_f32 _lacunarity) {
  if (_scale <= 0.0f) {
    _scale = 0.0001f;
  }

  vector<rx_f32> noise_map{static_cast<rx_size>(_dimensions.area())};
  vector<math::vec2f> octave_offsets{static_cast<rx_size>(_octaves)};
  for (rx_s32 i{0}; i < _octaves; i++) {
    const rx_f32 x{0.0f};
    const rx_f32 y{0.0f};
    octave_offsets[i] = {x, y};
  }

  rx_f32 max_noise_height{-FLT_MAX};
  rx_f32 min_noise_height{ FLT_MAX};

  // Generate from middle out.
  const rx_f32 half_w{_dimensions.x / 2.0f};
  const rx_f32 half_h{_dimensions.y / 2.0f};

  for (rx_s32 y{0}; y < _dimensions.h; y++) {
    for (rx_s32 x{0}; x < _dimensions.w; x++) {
      rx_f32 amplitude{1.0f};
      rx_f32 frequency{1.0f};
      rx_f32 noise_height{0.0f};

      for (rx_s32 i{0}; i < _octaves; i++) {
        const rx_f32 sample_x{(static_cast<rx_f32>(x) - half_w) / _scale * frequency + octave_offsets[i].x};
        const rx_f32 sample_y{(static_cast<rx_f32>(y) - half_h) / _scale * frequency + octave_offsets[i].y};

        const rx_f32 perlin_value{perlin_noise(sample_x, sample_y)};

        noise_height += perlin_value * amplitude;
        amplitude *= _persistance;
        frequency *= _lacunarity;
      }

      max_noise_height = algorithm::max(max_noise_height, noise_height);
      min_noise_height = algorithm::min(min_noise_height, noise_height);

      noise_map[_dimensions.w * y + x] = noise_height;
    }
  }

  // Normalize
  for (rx_s32 y{0}; y < _dimensions.h; y++) {
    for (rx_s32 x{0}; x < _dimensions.w; x++) {
      noise_map[_dimensions.w * y + x] = inverse_lerp(min_noise_height, max_noise_height, noise_map[_dimensions.w * y + x]);
    }
  }

  return noise_map;
}

static vector<math::vec3f> generate_map(const vector<terrain_type>& _terrain_types, const math::vec2i& _dimensions, rx_f32 _scale, int _octaves, rx_f32 _persistence, rx_f32 _lacunarity) {
  const auto& noise_map{generate_noise_map(_dimensions, _scale, _octaves, _persistence, _lacunarity)};
  vector<math::vec3f> color_map{static_cast<rx_size>(_dimensions.area())};
  for (rx_s32 y{0}; y < _dimensions.h; y++) {
    for (rx_s32 x{0}; x < _dimensions.w; x++) {
      const rx_f32 current_height{noise_map[_dimensions.w * y + x]};
      _terrain_types.each_fwd([&](const terrain_type& _terrain_type) {
        if (current_height <= _terrain_type.height) {
          color_map[_dimensions.w * y + x] = _terrain_type.color;
          return false;
        }
        return true;
      });
    }
  }
  return color_map;
}

struct terrain_mesh {
  struct vertex {
    math::vec3f position;
    math::vec2f coordinate;
  };

  vector<vertex> vertices;
  vector<rx_u32> triangles;
  rx_size triangle_index;

  terrain_mesh(const math::vec2i& _dimensions) {
    triangle_index = 0;
    vertices.resize(_dimensions.area());
    triangles.resize((_dimensions.w - 1) * (_dimensions.h - 1) * 3 * 2);
  }

  void add_triangle(rx_u32 _a, rx_u32 _b, rx_u32 _c) {
    triangles[triangle_index++] = _a;
    triangles[triangle_index++] = _b;
    triangles[triangle_index++] = _c;
  }
};

static terrain_mesh generate_terrain_mesh(const vector<rx_f32>& _height_map, const math::vec2i& _dimensions) {
  terrain_mesh mesh{_dimensions};
  rx_size vertex_index{0};
  const rx_f32 top_left_x{(_dimensions.w - 1) / -2.0f};
  const rx_f32 top_left_z{(_dimensions.h - 1) /  2.0f};
  for (rx_s32 y{0}; y < _dimensions.h; y++) {
    for (rx_s32 x{0}; x < _dimensions.w; x++) {
      const rx_f32 height{_height_map[_dimensions.w * y + x]};
      mesh.vertices[vertex_index++] = {
        {top_left_x + x, height, top_left_z - y},
        {x / static_cast<rx_f32>(_dimensions.w), y / static_cast<rx_f32>(_dimensions.h)}};
      if (x < _dimensions.w - 1 && y < _dimensions.h - 1) {
        mesh.add_triangle(vertex_index, vertex_index + -_dimensions.w + 1, vertex_index + _dimensions.w);
        mesh.add_triangle(vertex_index + _dimensions.w + 1, vertex_index, vertex_index + 1);
      }
    }
  }
  return mesh;
}

#endif

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
    m_model.load("base/models/chest/chestd.json5");
    return true;
  }

  virtual status on_slice(const input::input& _input) {
#if 0
    const math::vec2i& noise_dimensions{100, 100};
    const rx_f32 noise_scale{100.0f};
    const rx_s32 octaves{4};
    const rx_f32 persistence{0.5f};
    const rx_f32 lacunarity{2.0f};

    auto rgb = [](int r, int g, int b) {
      return math::vec3f{r/255.0f, g/255.0f, b/255.0f};
    };

    //const auto& noise_map{generate_noise_map(noise_dimensions, noise_scale, octaves, persistence, lacunarity)};
    //write_noise_map(noise_map, noise_dimensions);
    vector<terrain_type> terrain{8};
    terrain[0].height = 0.1f;
    terrain[0].color = rgb(51, 99, 193);

    terrain[1].height = 0.2f;
    terrain[1].color = rgb(54, 102, 198);

    terrain[2].height = 0.4f;
    terrain[2].color = rgb(207, 209, 127);

    terrain[3].height = 0.55f;
    terrain[3].color = rgb(85, 152, 22);

    terrain[4].height = 0.6f;
    terrain[4].color = rgb(63, 106, 19);

    terrain[5].height = 0.7f;
    terrain[5].color = rgb(92, 69, 63);

    terrain[6].height = 0.9f;
    terrain[6].color = rgb(74, 60, 55);

    terrain[7].height = 1.0f;
    terrain[7].color = rgb(255, 255, 255);

    const auto& terrain_map{generate_noise_map(noise_dimensions, noise_scale, octaves, persistence, lacunarity)};
    // write_color_map(terrain_map, noise_dimensions);
    generate_terrain_mesh(terrain_map, noise_dimensions);

    abort();
#endif

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
      return status::k_shutdown;
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

    return status::k_running;
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
