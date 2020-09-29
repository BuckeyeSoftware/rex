#include "rx/game.h"

#include "rx/render/frontend/context.h"
#include "rx/render/frontend/target.h"
#include "rx/render/frontend/downloader.h"

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

#include "rx/console/context.h"
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
    , m_ibl{&m_frontend}
    , m_indirect_lighting_pass{&m_frontend, &m_gbuffer, &m_ibl}
    , m_lens_distortion_pass{&m_frontend}
  {
  }

  ~TestGame() {
  }

  virtual bool on_init() {
    m_gbuffer.create(m_frontend.swapchain()->dimensions());
    m_skybox.load("base/skyboxes/yokohama/yokohama.json5", {1024, 1024});
    m_ibl.render(m_skybox.cubemap(), 256);

    m_indirect_lighting_pass.create(m_frontend.swapchain()->dimensions());
    m_lens_distortion_pass.create(m_frontend.swapchain()->dimensions());

    Render::Model model{&m_frontend};
    if (model.load("base/models/san-miguel/san-miguel.json5")) {
      m_models.push_back(Utility::move(model));
    }

    m_models.each_fwd([](Render::Model& _model) {
      _model.animate(0, true);
    });

    return true;
  }

  virtual bool on_update(Console::Context& console_, Input::Context& input_) {
    static auto display_resolution =
      console_.find_variable_by_name("display.resolution")->cast<Math::Vec2i>();
    static auto display_swap_interval =
      console_.find_variable_by_name("display.swap_interval")->cast<Sint32>();
    static auto display_fullscreen =
      console_.find_variable_by_name("display.fullscreen")->cast<Sint32>();

    const auto& dimensions{display_resolution->get().cast<Float32>()};
    m_camera.projection = Math::Mat4x4f::perspective(90.0f, {0.01f, 2048.0f},
      dimensions.w / dimensions.h);

    if (!input_.active_text()) {
      Float32 move_speed{0.0f};
      const Float32 sens{0.2f};
      const auto &delta = input_.mouse().movement();

      Math::Vec3f move{static_cast<Float32>(delta.y) * sens, static_cast<Float32>(delta.x) * sens, 0.0f};
      m_camera.rotate = m_camera.rotate + move;

      if (input_.keyboard().is_held(Input::ScanCode::k_left_control)) {
        move_speed = 10.0f;
      } else {
        move_speed = 5.0f;
      }

      if (input_.keyboard().is_held(Input::ScanCode::k_w)) {
        const auto f{m_camera.as_mat4().z};
        m_camera.translate += Math::Vec3f(f.x, f.y, f.z) * (move_speed * m_frontend.timer().delta_time());
      }
      if (input_.keyboard().is_held(Input::ScanCode::k_s)) {
        const auto f{m_camera.as_mat4().z};
        m_camera.translate -= Math::Vec3f(f.x, f.y, f.z) * (move_speed * m_frontend.timer().delta_time());
      }
      if (input_.keyboard().is_held(Input::ScanCode::k_d)) {
        const auto l{m_camera.as_mat4().x};
        m_camera.translate += Math::Vec3f(l.x, l.y, l.z) * (move_speed * m_frontend.timer().delta_time());
      }
      if (input_.keyboard().is_held(Input::ScanCode::k_a)) {
        const auto l{m_camera.as_mat4().x};
        m_camera.translate -= Math::Vec3f(l.x, l.y, l.z) * (move_speed * m_frontend.timer().delta_time());
      }
    }

    if (input_.keyboard().is_released(Input::ScanCode::k_f1)) {
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

    if (input_.keyboard().is_released(Input::ScanCode::k_escape)) {
      return false;
    }

    if (input_.keyboard().is_released(Input::ScanCode::k_f12)) {
      display_fullscreen->set((display_fullscreen->get() + 1) % 3);
    }

    if (input_.keyboard().is_released(Input::ScanCode::k_f11)) {
      const auto &name{m_skybox.name()};
      const char* next{nullptr};
      /**/ if (name == "miramar")  next = "base/skyboxes/nebula/nebula.json5";
      else if (name == "nebula")   next = "base/skyboxes/yokohama/yokohama.json5";
      else if (name == "yokohama") next = "base/skyboxes/miramar/miramar.json5";
      m_skybox.load(next, {1024, 1024});
      m_ibl.render(m_skybox.cubemap(), 256);
    }

    m_console.update(console_, input_);

    return true;
  }

  virtual bool on_render(Console::Context& console_) {
    static auto display_resolution =
      console_.find_variable_by_name("display.resolution")->cast<Math::Vec2i>();

    Render::Frontend::State state;
    state.viewport.record_dimensions(display_resolution->get().cast<Size>());

    Render::Frontend::Buffers draw_buffers;
    draw_buffers.add(0);
    draw_buffers.add(1);
    draw_buffers.add(2);

    m_frontend.clear(
      RX_RENDER_TAG("gbuffer"),
      state,
      m_gbuffer.target(),
      draw_buffers,
      RX_RENDER_CLEAR_DEPTH |
      RX_RENDER_CLEAR_STENCIL |
      RX_RENDER_CLEAR_COLOR(0) |
      RX_RENDER_CLEAR_COLOR(1) |
      RX_RENDER_CLEAR_COLOR(2),
      1.0f,
      0,
      Math::Vec4f{1.0f, 1.0f, 1.0f, 1.0f}.data(),
      Math::Vec4f{1.0f, 1.0f, 1.0f, 1.0f}.data(),
      Math::Vec4f{1.0f, 1.0f, 1.0f, 1.0f}.data());

    m_models.each_fwd([&](Render::Model& model_) {
      model_.update(m_frontend.timer().delta_time());
      model_.render(m_gbuffer.target(), {}, m_camera.view(), m_camera.projection);
      model_.render_skeleton({}, &m_immediate3D);
    });

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

    m_console.render();

    m_immediate2D.render(m_frontend.swapchain());

    return true;
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
  Vector<Render::Model> m_models;

  Render::ImageBasedLighting m_ibl;

  Render::IndirectLightingPass m_indirect_lighting_pass;
  Render::LensDistortionPass m_lens_distortion_pass;

  Math::Camera m_camera;
};

Ptr<Game> create(Render::Frontend::Context& _frontend) {
  return make_ptr<TestGame>(Memory::SystemAllocator::instance(), _frontend);
}
