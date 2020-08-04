#include "rx/game.h"

#include "rx/render/frontend/context.h"
#include "rx/render/frontend/target.h"

#include "rx/render/immediate2D.h"
#include "rx/render/immediate3D.h"
#include "rx/render/gbuffer.h"
#include "rx/render/image_based_lighting.h"
#include "rx/render/skybox.h"
#include "rx/render/model.h"

#include "rx/render/indirect_lighting_pass.h"
#include "rx/render/lens_distortion_pass.h"

#include "rx/hud/console.h"
#include "rx/hud/frame_graph.h"
#include "rx/hud/memory_stats.h"
#include "rx/hud/render_stats.h"

#include "rx/console/interface.h"
#include "rx/console/variable.h"

#include "rx/math/camera.h"

using namespace Rx;

RX_CONSOLE_FVAR(
  lens_distortion,
  "lens.distortion",
  "lens distortion",
  0.0f,
  1.0f,
  0.1f);

RX_CONSOLE_FVAR(
  lens_scale,
  "lens.scale",
  "lens scale",
  0.0f,
  1.0f,
  0.9f);

RX_CONSOLE_FVAR(
  lens_dispersion,
  "lens.dispersion",
  "lens dispersion",
  0.0f,
  1.0f,
  0.01f);

struct TestGame
  : Game
{
  TestGame(Render::Frontend::Context& _frontend)
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
    , m_indirect_lighting_pass{&m_frontend, &m_gbuffer, &m_ibl}
    , m_lens_distortion_pass{&m_frontend}
  {
    model_transform[0].translate = {-5.0f, 0.0f, 0.0f};
    model_transform[0].scale     = { 2.0f, 2.0f, 2.0f};

    model_transform[2].translate = { 5.0f, 0.0f, 0.0f};
  }

  ~TestGame() {
  }

  virtual bool on_init() {
    m_gbuffer.create(m_frontend.swapchain()->dimensions());
    m_skybox.load("base/skyboxes/yokohama/yokohama.json5");
    m_model0.load("base/models/chest/chest.json5");
    m_model1.load("base/models/fire_hydrant/fire_hydrant.json5");
    m_model2.load("base/models/mrfixit/mrfixit.json5");
    m_ibl.render(m_skybox.cubemap(), 256);

    m_indirect_lighting_pass.create(m_frontend.swapchain()->dimensions());
    m_lens_distortion_pass.create(m_frontend.swapchain()->dimensions());

    m_model0.animate(0, true);
    m_model1.animate(0, true);
    m_model2.animate(0, true);

    return true;
  }

  virtual Status on_slice(Input::Context& _input) {
#if 0
    const math::vec2i& noise_dimensions{100, 100};
    const Float32 noise_scale{100.0f};
    const Sint32 octaves{4};
    const Float32 persistence{0.5f};
    const Float32 lacunarity{2.0f};

    auto rgb = [](int r, int g, int b) {
      return math::vec3f{r/255.0f, g/255.0f, b/255.0f};
    };

    //const auto& noise_map{generate_noise_map(noise_dimensions, noise_scale, octaves, persistence, lacunarity)};
    //write_noise_map(noise_map, noise_dimensions);
    Vector<terrain_type> terrain{8};
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

    static auto display_resolution{Console::Interface::find_variable_by_name("display.resolution")->cast<Math::Vec2i>()};
    static auto display_swap_interval{Console::Interface::find_variable_by_name("display.swap_interval")->cast<Sint32>()};
    static auto display_fullscreen{Console::Interface::find_variable_by_name("display.fullscreen")->cast<Sint32>()};

    const auto& dimensions{display_resolution->get().cast<Float32>()};
    m_camera.projection = Math::Mat4x4f::perspective(90.0f, {0.01f, 2048.0f},
      dimensions.w / dimensions.h);

    if (!_input.active_text()) {
      Float32 move_speed{0.0f};
      const Float32 sens{0.2f};
      const auto &delta{_input.mouse().movement()};


      Math::Vec3f move{static_cast<Float32>(delta.y) * sens, static_cast<Float32>(delta.x) * sens, 0.0f};
      m_camera.rotate = m_camera.rotate + move;

      if (_input.keyboard().is_held(Input::ScanCode::k_left_control)) {
        move_speed = 10.0f;
      } else {
        move_speed = 5.0f;
      }

      if (_input.keyboard().is_held(Input::ScanCode::k_w)) {
        const auto f{m_camera.to_mat4().z};
        m_camera.translate += Math::Vec3f(f.x, f.y, f.z) * (move_speed * m_frontend.timer().delta_time());
      }
      if (_input.keyboard().is_held(Input::ScanCode::k_s)) {
        const auto f{m_camera.to_mat4().z};
        m_camera.translate -= Math::Vec3f(f.x, f.y, f.z) * (move_speed * m_frontend.timer().delta_time());
      }
      if (_input.keyboard().is_held(Input::ScanCode::k_d)) {
        const auto l{m_camera.to_mat4().x};
        m_camera.translate += Math::Vec3f(l.x, l.y, l.z) * (move_speed * m_frontend.timer().delta_time());
      }
      if (_input.keyboard().is_held(Input::ScanCode::k_a)) {
        const auto l{m_camera.to_mat4().x};
        m_camera.translate -= Math::Vec3f(l.x, l.y, l.z) * (move_speed * m_frontend.timer().delta_time());
      }
    }

    if (_input.keyboard().is_released(Input::ScanCode::k_f1)) {
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

    if (_input.keyboard().is_released(Input::ScanCode::k_escape)) {
      return Status::k_shutdown;
    }

    if (_input.keyboard().is_released(Input::ScanCode::k_f12)) {
      display_fullscreen->set((display_fullscreen->get() + 1) % 3);
    }

    if (_input.keyboard().is_released(Input::ScanCode::k_f11)) {
      const auto &name{m_skybox.name()};
      const char* next{nullptr};
      /**/ if (name == "miramar")  next = "base/skyboxes/nebula/nebula.json5";
      else if (name == "nebula")   next = "base/skyboxes/yokohama/yokohama.json5";
      else if (name == "yokohama") next = "base/skyboxes/miramar/miramar.json5";
      m_skybox.load(next);
      m_ibl.render(m_skybox.cubemap(), 256);
    }

    Render::Frontend::State state;
    state.viewport.record_dimensions(display_resolution->get().cast<Size>());

    Render::Frontend::Buffers draw_buffers;
    draw_buffers.add(0);
    draw_buffers.add(1);
    draw_buffers.add(2);
    draw_buffers.add(3);

    m_frontend.clear(
            RX_RENDER_TAG("gbuffer"),
            state,
            m_gbuffer.target(),
            draw_buffers,
      RX_RENDER_CLEAR_DEPTH |
      RX_RENDER_CLEAR_STENCIL |
      RX_RENDER_CLEAR_COLOR(0) |
      RX_RENDER_CLEAR_COLOR(1) |
      RX_RENDER_CLEAR_COLOR(2) |
      RX_RENDER_CLEAR_COLOR(3),
            1.0f,
            0,
            Math::Vec4f{0.0f, 0.0f, 0.0f, 0.0f}.data(),
            Math::Vec4f{0.0f, 0.0f, 0.0f, 0.0f}.data(),
            Math::Vec4f{0.0f, 0.0f, 0.0f, 0.0f}.data(),
            Math::Vec4f{0.0f, 0.0f, 0.0f, 0.0f}.data());

    // model_xform.rotate += math::vec3f(0.0f, 20.0f, 0.0f) * ;
    m_model0.update(m_frontend.timer().delta_time());
    m_model1.update(m_frontend.timer().delta_time());
    m_model2.update(m_frontend.timer().delta_time());

    m_model0.render(m_gbuffer.target(), model_transform[0].to_mat4(), m_camera.view(), m_camera.projection);
    m_model1.render(m_gbuffer.target(), model_transform[1].to_mat4(), m_camera.view(), m_camera.projection);
    m_model2.render(m_gbuffer.target(), model_transform[2].to_mat4(), m_camera.view(), m_camera.projection);

    m_indirect_lighting_pass.render(m_camera);

    // Render the skybox absolutely last.
    m_skybox.render(m_indirect_lighting_pass.target(), m_camera.view(), m_camera.projection);

    // Then 3D immediates.
    m_immediate3D.render(m_indirect_lighting_pass.target(), m_camera.view(), m_camera.projection);

    // Lens distortion pass.
    m_lens_distortion_pass.distortion = *lens_distortion;
    m_lens_distortion_pass.dispersion = *lens_dispersion;
    m_lens_distortion_pass.scale = *lens_scale;
    m_lens_distortion_pass.render(m_indirect_lighting_pass.texture());

    // Blit lens distortion pass to backbuffer
    m_frontend.blit(
      RX_RENDER_TAG("test"),
      state,
      m_lens_distortion_pass.target(),
      0,
      m_frontend.swapchain(),
      0);

    m_frame_graph.render();
    m_render_stats.render();
    m_memory_stats.render();

    m_console.update(_input);
    m_console.render();

    m_immediate2D.render(m_frontend.swapchain());

    return Status::k_running;
  }

  void on_resize(const Math::Vec2z& _dimensions) {
    m_gbuffer.resize(_dimensions);
    m_indirect_lighting_pass.resize(_dimensions);
    m_frontend.resize(_dimensions);
  }

  Render::Frontend::Context& m_frontend;

  Render::Immediate2D m_immediate2D;
  Render::Immediate3D m_immediate3D;

  hud::Console m_console;
  hud::FrameGraph m_frame_graph;
  hud::MemoryStats m_memory_stats;
  hud::RenderStats m_render_stats;

  Render::GBuffer m_gbuffer;
  Render::Skybox m_skybox;
  Render::Model m_model0;
  Render::Model m_model1;
  Render::Model m_model2;

  Render::ImageBasedLighting m_ibl;

  Render::IndirectLightingPass m_indirect_lighting_pass;
  Render::LensDistortionPass m_lens_distortion_pass;

  Math::Transform model_transform[3];
  Math::Camera m_camera;
};

Ptr<Game> create(Render::Frontend::Context& _frontend) {
  return make_ptr<TestGame>(Memory::SystemAllocator::instance(), _frontend);
}
