#include <SDL.h> // TODO remove

#include "rx/game.h"

#include "rx/render/frontend/interface.h"
#include "rx/render/frontend/technique.h"
#include "rx/render/frontend/buffer.h"
#include "rx/render/frontend/target.h"

#include "rx/render/gbuffer.h"
#include "rx/render/ibl.h"
#include "rx/render/immediate2D.h"
#include "rx/render/immediate3D.h"
#include "rx/render/skybox.h"
#include "rx/render/model.h"

#include "rx/hud/console.h"
#include "rx/hud/frame_graph.h"
#include "rx/hud/memory_stats.h"
#include "rx/hud/render_stats.h"

#include "rx/console/interface.h"

#include "rx/math/camera.h"

using namespace rx;

static render::frontend::buffer* quad_buffer(render::frontend::interface* _frontend) {
  render::frontend::buffer* buffer{_frontend->cached_buffer("quad")};
  if (buffer) {
    return buffer;
  }

  static constexpr const struct vertex {
    math::vec2f position;
    math::vec2f coordinate;
  } k_quad_vertices[]{
    {{-1.0f,  1.0f}, {0.0f, 1.0f}},
    {{ 1.0f,  1.0f}, {1.0f, 1.0f}},
    {{-1.0f, -1.0f}, {0.0f, 0.0f}},
    {{ 1.0f, -1.0f}, {1.0f, 0.0f}}
  };

  buffer = _frontend->create_buffer(RX_RENDER_TAG("quad"));
  buffer->record_type(render::frontend::buffer::type::k_static);
  buffer->record_element_type(render::frontend::buffer::element_type::k_none);
  buffer->record_stride(sizeof(vertex));
  buffer->record_attribute(render::frontend::buffer::attribute::type::k_f32, 2, offsetof(vertex, position));
  buffer->record_attribute(render::frontend::buffer::attribute::type::k_f32, 2, offsetof(vertex, coordinate));
  buffer->write_vertices(k_quad_vertices, sizeof k_quad_vertices);

  _frontend->initialize_buffer(RX_RENDER_TAG("quad"), buffer);
  _frontend->cache_buffer(buffer, "quad");

  return buffer;
}

struct test_game
  : game
{
  test_game(render::frontend::interface& _frontend)
    : m_frontend{_frontend}
    , m_immediate2D{&m_frontend}
    , m_immediate3D{&m_frontend}
    , m_console{&m_immediate2D}
    , m_frame_graph{&m_immediate2D}
    , m_memory_stats{&m_immediate2D}
    , m_render_stats{&m_immediate2D}
    , m_gbuffer{&m_frontend}
    , m_skybox{&m_frontend}
    , m_model0{&m_frontend}
    , m_model1{&m_frontend}
    , m_model2{&m_frontend}
    , m_ibl{&m_frontend}
    , m_di{m_frontend.create_target(RX_RENDER_TAG("di"))}
    , m_dic{m_frontend.create_texture2D(RX_RENDER_TAG("dic"))}
    , m_dit{m_frontend.find_technique_by_name("deferred_indirect")}
    , m_b{quad_buffer(&m_frontend)}
  {
    model_transform[0].translate = {-5.0f, 0.0f, 0.0f};
    model_transform[0].scale     = { 2.0f, 2.0f, 2.0f};

    model_transform[2].translate = { 5.0f, 0.0f, 0.0f};
  }

  ~test_game() {
    m_frontend.destroy_target(RX_RENDER_TAG("di"), m_di);
    m_frontend.destroy_texture(RX_RENDER_TAG("dic"), m_dic);
    m_frontend.destroy_buffer(RX_RENDER_TAG("b"), m_b);
  }

  virtual bool on_init() {
    m_gbuffer.create(m_frontend.swapchain()->dimensions());
    m_skybox.load("base/skyboxes/yokohama/yokohama.json5");
    m_model0.load("base/models/chest/chest.json5");
    m_model1.load("base/models/fellord/fellord.json5");
    m_model2.load("base/models/mrfixit/mrfixit.json5");
    m_ibl.render(m_skybox.cubemap(), 256);

    static auto display_resolution{console::interface::find_variable_by_name("display.resolution")->cast<math::vec2i>()};
    m_dic->record_type(render::frontend::texture::type::k_attachment);
    m_dic->record_format(render::frontend::texture::data_format::k_rgb_u8);
    m_dic->record_filter({false, false, false});
    m_dic->record_levels(1);
    m_dic->record_dimensions(display_resolution->get().cast<rx_size>());
    m_dic->record_wrap({
      render::frontend::texture::wrap_type::k_clamp_to_edge,
      render::frontend::texture::wrap_type::k_clamp_to_edge});
    m_frontend.initialize_texture(RX_RENDER_TAG("dic"), m_dic);

    m_di->attach_texture(m_dic, 0);
    m_di->attach_depth_stencil(m_gbuffer.depth_stencil());
    m_frontend.initialize_target(RX_RENDER_TAG("di"), m_di);

    m_model0.animate(0, true);
    m_model1.animate(0, true);
    m_model2.animate(0, true);

    return true;
  }

  virtual status on_slice(input::input& _input) {
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

    static auto display_resolution{console::interface::find_variable_by_name("display.resolution")->cast<math::vec2i>()};
    static auto display_swap_interval{console::interface::find_variable_by_name("display.swap_interval")->cast<rx_s32>()};
    static auto display_fullscreen{console::interface::find_variable_by_name("display.fullscreen")->cast<rx_s32>()};

    const auto& dimensions{display_resolution->get().cast<rx_f32>()};
    m_camera.projection = math::mat4x4f::perspective(90.0f, {0.01f, 2048.0f},
      dimensions.w / dimensions.h);

    rx_f32 move_speed{0.0f};
    const rx_f32 sens{0.2f};
    const auto &delta{_input.mouse().movement()};
    math::vec3f move{static_cast<rx_f32>(delta.y) * sens, static_cast<rx_f32>(delta.x) * sens, 0.0f};
    m_camera.rotate = m_camera.rotate + move;

    if (_input.keyboard().is_held(input::scan_code::k_left_control)) {
      move_speed = 10.0f;
    } else {
      move_speed = 5.0f;
    }

    if (_input.keyboard().is_held(input::scan_code::k_w)) {
      const auto f{m_camera.to_mat4().z};
      m_camera.translate += math::vec3f(f.x, f.y, f.z) * (move_speed * m_frontend.timer().delta_time());
    }
    if (_input.keyboard().is_held(input::scan_code::k_s)) {
      const auto f{m_camera.to_mat4().z};
      m_camera.translate -= math::vec3f(f.x, f.y, f.z) * (move_speed * m_frontend.timer().delta_time());
    }
    if (_input.keyboard().is_held(input::scan_code::k_d)) {
      const auto l{m_camera.to_mat4().x};
      m_camera.translate += math::vec3f(l.x, l.y, l.z) * (move_speed * m_frontend.timer().delta_time());
    }
    if (_input.keyboard().is_held(input::scan_code::k_a)) {
      const auto l{m_camera.to_mat4().x};
      m_camera.translate -= math::vec3f(l.x, l.y, l.z) * (move_speed * m_frontend.timer().delta_time());
    }

    if (_input.keyboard().is_released(input::scan_code::k_f1)) {
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

    if (_input.keyboard().is_released(input::scan_code::k_escape)) {
      return status::k_shutdown;
    }

    if (_input.keyboard().is_released(input::scan_code::k_f12)) {
      display_fullscreen->set((display_fullscreen->get() + 1) % 3);
    }

    if (_input.keyboard().is_released(input::scan_code::k_f11)) {
      const auto &name{m_skybox.name()};
      const char* next{nullptr};
      /**/ if (name == "miramar")  next = "base/skyboxes/nebula/nebula.json5";
      else if (name == "nebula")   next = "base/skyboxes/yokohama/yokohama.json5";
      else if (name == "yokohama") next = "base/skyboxes/miramar/miramar.json5";
      m_skybox.load(next);
      m_ibl.render(m_skybox.cubemap(), 256);
    }

    render::frontend::state state;
    state.viewport.record_dimensions(display_resolution->get().cast<rx_size>());

    //state.depth.record_test(false);
    //state.depth.record_write(true);

    // state.stencil.record_enable(true);
    state.stencil.record_write_mask(0xff);

    m_frontend.clear(
      RX_RENDER_TAG("gbuffer"),
      state,
      m_gbuffer.target(),
      "0123",
      RX_RENDER_CLEAR_DEPTH |
      RX_RENDER_CLEAR_STENCIL |
      RX_RENDER_CLEAR_COLOR(0) |
      RX_RENDER_CLEAR_COLOR(1) |
      RX_RENDER_CLEAR_COLOR(2) |
      RX_RENDER_CLEAR_COLOR(3),
      1.0f,
      0,
      math::vec4f{0.0f, 0.0f, 0.0f, 0.0f}.data(),
      math::vec4f{0.0f, 0.0f, 0.0f, 0.0f}.data(),
      math::vec4f{0.0f, 0.0f, 0.0f, 0.0f}.data(),
      math::vec4f{0.0f, 0.0f, 0.0f, 0.0f}.data());

    // model_xform.rotate += math::vec3f(0.0f, 20.0f, 0.0f) * ;
    m_model0.update(m_frontend.timer().delta_time());
    m_model1.update(m_frontend.timer().delta_time());
    m_model2.update(m_frontend.timer().delta_time());

    m_model0.render(m_gbuffer.target(), model_transform[0].to_mat4(), m_camera.view(), m_camera.projection);
    m_model1.render(m_gbuffer.target(), model_transform[1].to_mat4(), m_camera.view(), m_camera.projection);
    m_model2.render(m_gbuffer.target(), model_transform[2].to_mat4(), m_camera.view(), m_camera.projection);

    // m_model0.render_normals(model_transform[0].to_mat4(), &m_immediate3D);
    // m_model1.render_normals(model_transform[1].to_mat4(), &m_immediate3D);
    // m_model2.render_normals(model_transform[2].to_mat4(), &m_immediate3D);

    // m_model0.render_skeleton(model_transform[0].to_mat4(), &m_immediate3D);
    // m_model1.render_skeleton(model_transform[1].to_mat4(), &m_immediate3D);
    // m_model2.render_skeleton(model_transform[2].to_mat4(), &m_immediate3D);

    // m_model.render_normals(model_xform.to_mat4(), &m_immediate3D);

    // m_immediate3D.render(m_gbuffer.target(), m_camera.view(), m_camera.projection);

    state.viewport.record_dimensions(display_resolution->get().cast<rx_size>());

    rx::render::frontend::program* p = *m_dit;
    p->uniforms()[6].record_mat4x4f(math::mat4x4f::invert(m_camera.view() * m_camera.projection));
    p->uniforms()[7].record_vec3f(m_camera.translate);

    m_frontend.clear(
      RX_RENDER_TAG(""),
      state,
      m_di,
      "0",
      RX_RENDER_CLEAR_COLOR(0),
      math::vec4f{0.0f, 0.0f, 0.0f, 1.0f}.data());

    // Only apply indirect lighting to areas in the stencil filled with 1s
    // glEnable(GL_STENCIL_TEST)
    //state.stencil.record_enable(false);

    // glStencilFunc(GL_EQUAL, 1, 0xFF)
    // state.stencil.record_function(render::frontend::stencil_state::function_type::k_equal);
    // state.stencil.record_reference(1);
    // state.stencil.record_mask(0xff);

    // glStencilMask(0x00)
    // state.stencil.record_write_mask(0x00);

    // state.stencil.record_fail_action(render::frontend::stencil_state::operation_type::k_keep);
    // state.stencil.record_depth_fail_action(render::frontend::stencil_state::operation_type::k_keep);
    // state.stencil.record_depth_pass_action(render::frontend::stencil_state::operation_type::k_replace);

    m_frontend.draw(
      RX_RENDER_TAG("di"),
      state,
      m_di,
      "0",
      m_b,
      p,
      4,
      0,
      render::frontend::primitive_type::k_triangle_strip,
      "222cc2",
      m_gbuffer.albedo(),
      m_gbuffer.normal(),
      m_gbuffer.depth_stencil(),
      m_ibl.irradiance(),
      m_ibl.prefilter(),
      m_ibl.scale_bias());

    // Render the skybox absolutely last.
    m_skybox.render(m_di, m_camera.view(), m_camera.projection);

    // Then 3D immediates.
    m_immediate3D.render(m_di, m_camera.view(), m_camera.projection);

    // Blit indirect diffuse pass to backbuffer
    m_frontend.blit(
      RX_RENDER_TAG("test"),
      state,
      m_di,
      0,
      m_frontend.swapchain(),
      0);

    m_frame_graph.render();
    m_render_stats.render();
    m_memory_stats.render();

    m_console.update(_input);
    m_console.render();

    m_immediate2D.render(m_frontend.swapchain());

    return status::k_running;
  }

  void on_resize(const math::vec2z& _dimensions) {
    m_gbuffer.resize(_dimensions);
    m_frontend.resize(_dimensions);
  }

  render::frontend::interface& m_frontend;

  render::immediate2D m_immediate2D;
  render::immediate3D m_immediate3D;

  hud::console m_console;
  hud::frame_graph m_frame_graph;
  hud::memory_stats m_memory_stats;
  hud::render_stats m_render_stats;

  render::gbuffer m_gbuffer;
  render::skybox m_skybox;
  render::model m_model0;
  render::model m_model1;
  render::model m_model2;

  render::ibl m_ibl;

  // TODO(dweiler): indirect diffuse renderable
  render::frontend::target* m_di;
  render::frontend::texture2D* m_dic;
  render::frontend::technique* m_dit;
  render::frontend::buffer* m_b;

  math::transform model_transform[3];
  math::camera m_camera;
};

game* create(render::frontend::interface& _frontend) {
  return memory::g_system_allocator->create<test_game>(_frontend);
}
