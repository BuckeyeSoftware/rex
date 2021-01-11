#include <stdio.h>

#include "rx/application.h"
#include "rx/engine.h"

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
#include "rx/render/copy_pass.h"
#include "rx/render/color_grader.h"

#include "rx/hud/console.h"
#include "rx/hud/frame_graph.h"
#include "rx/hud/memory_stats.h"
#include "rx/hud/render_stats.h"

#include "rx/console/context.h"
#include "rx/console/variable.h"

#include "rx/math/camera.h"

#include "rx/core/filesystem/directory.h"

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

RX_CONSOLE_BVAR(
  input_debug_layers,
  "input.debug_layers",
  "draw debug rectangles for each input layer",
  false);

struct TestGame
  : Application
{
  TestGame(Engine* _engine)
    : Application(_engine)
    , m_frontend{*engine()->renderer()}
    , m_immediate2D{&m_frontend}
    , m_immediate3D{&m_frontend}
    , m_console{&m_immediate2D, engine()->input()}
    , m_frame_graph{&m_immediate2D}
    , m_memory_stats{&m_immediate2D}
    , m_render_stats{&m_immediate2D}
    , m_gbuffer{&m_frontend}
    , m_skybox{&m_frontend}
    , m_lens_distortion_pass{&m_frontend}
    , m_color_grader{&m_frontend}
    , m_lut_index{0}
    , m_lut_count{0}
  {
    engine()->input().root_layer().raise();
  }

  ~TestGame() {
  }

  virtual bool on_init() {
    m_gbuffer.create(m_frontend.swapchain()->dimensions());
    m_skybox.load("base/skyboxes/miramar/miramar.json5", {1024, 1024});

    auto indirect_lighting_pass
      = Render::IndirectLightingPass::create(&m_frontend, m_frontend.swapchain()->dimensions());
    if (!indirect_lighting_pass) {
      return false;
    }

    m_indirect_lighting_pass = Utility::move(*indirect_lighting_pass);

    // m_indirect_lighting_pass.create(m_frontend.swapchain()->dimensions());
    m_lens_distortion_pass.create(m_frontend.swapchain()->dimensions());

    auto copy_pass = Render::CopyPass::create(&m_frontend,
      m_frontend.swapchain()->dimensions(), m_gbuffer.depth_stencil());
    if (!copy_pass) {
      return false;
    }

    m_copy_pass = Utility::move(*copy_pass);

    const char* LUTS[] = {
      "base/colorgrading/Arabica 12.CUBE",
      "base/colorgrading/Ava 614.CUBE",
      "base/colorgrading/Azrael 93.CUBE",
      "base/colorgrading/Bourbon 64.CUBE",
      "base/colorgrading/Byers 11.CUBE",
      "base/colorgrading/Chemical 168.CUBE",
      "base/colorgrading/Clayton 33.CUBE",
      "base/colorgrading/Clouseau 54.CUBE",
      "base/colorgrading/Cobi 3.CUBE",
      "base/colorgrading/Contrail 35.CUBE",
      "base/colorgrading/Cubicle 99.CUBE",
      "base/colorgrading/Django 25.CUBE",
      "base/colorgrading/Domingo 145.CUBE",
      "base/colorgrading/Faded 47.CUBE",
      "base/colorgrading/Folger 50.CUBE",
      "base/colorgrading/Fusion 88.CUBE",
      "base/colorgrading/Hyla 68.CUBE",
      "base/colorgrading/Korben 214.CUBE",
      "base/colorgrading/Lenox 340.CUBE",
      "base/colorgrading/Lucky 64.CUBE",
      "base/colorgrading/McKinnon 75.CUBE",
      "base/colorgrading/Milo 5.CUBE",
      "base/colorgrading/Neon 770.CUBE",
      "base/colorgrading/Paladin 1875.CUBE",
      "base/colorgrading/Pasadena 21.CUBE",
      "base/colorgrading/Pitaya 15.CUBE",
      "base/colorgrading/Reeve 38.CUBE",
      "base/colorgrading/Remy 24.CUBE",
      "base/colorgrading/Sprocket 231.CUBE",
      "base/colorgrading/Teigen 28.CUBE",
      "base/colorgrading/Trent 18.CUBE",
      "base/colorgrading/Tweed 71.CUBE",
      "base/colorgrading/Vireo 37.CUBE",
      "base/colorgrading/Zed 32.CUBE",
      "base/colorgrading/Zeke 39.CUBE"
    };

    auto ibl = Render::ImageBasedLighting::create(&m_frontend, 16, 64);
    if (!ibl) {
      return false;
    }

    m_ibl = Utility::move(*ibl);

    // Load in all the grading LUTs and update the atlas.
    for (Size i = 0; i < sizeof LUTS / sizeof* LUTS; i++) {
      if (auto lut = m_color_grader.load(LUTS[i])) {
        m_luts[m_lut_count++] = Utility::move(*lut);
      }
    }
    m_color_grader.update();


/*
    if (Filesystem::Directory dir{"base/models/light_unit"}) {
      printf("opened light_unit\n");
      dir.each([&](Filesystem::Directory::Item&& item_) {
        if (item_.is_file() && item_.name().ends_with(".json5")) {
          printf("found %s\n", item_.name().data());
          Render::Model model{&m_frontend};
          if (model.load("base/models/light_unit/" + item_.name())) {
            m_models.push_back(Utility::move(model));
          }
        }
      });
    } else {
      return false;
    }*/

    Render::Model model1{&m_frontend};
    Render::Model model2{&m_frontend};
    if (model1.load("base/models/mrfixit/mrfixit.json5")) {
      if (!m_models.push_back(Utility::move(model1))) {
        return false;
      }
    }

    if (model2.load("base/models/fire_hydrant/fire_hydrant.json5")) {
      if (!m_models.push_back(Utility::move(model2))) {
        return false;
      }
    }

    /*m_models.each_fwd([](Render::Model& _model) {
      _model.animate(0, true);
    });*/

    auto animate = [](Render::Model& _model) { _model.animate(0, true); };
    m_models.each_fwd(animate);

    return true;
  }

  virtual bool on_update(Float32 _delta_time) {
    auto& input = engine()->input();
    auto& console = engine()->console();

    static auto display_resolution =
      console.find_variable_by_name("display.resolution")->cast<Math::Vec2i>();
    static auto display_swap_interval =
      console.find_variable_by_name("display.swap_interval")->cast<Sint32>();
    static auto display_fullscreen =
      console.find_variable_by_name("display.fullscreen")->cast<Sint32>();

    const auto& dimensions{display_resolution->get().cast<Float32>()};
    m_camera.projection = Math::Mat4x4f::perspective(90.0f, {0.01f, 2048.0f},
      dimensions.w / dimensions.h);

    if (input.root_layer().is_active()) {
      Float32 move_speed{0.0f};
      const Float32 sens{0.2f};
      const auto &delta = input.root_layer().mouse().movement();

      Math::Vec3f move{static_cast<Float32>(delta.y) * sens, static_cast<Float32>(delta.x) * sens, 0.0f};
      m_camera.rotate = m_camera.rotate + move;

      if (input.root_layer().keyboard().is_held(Input::ScanCode::LEFT_CONTROL)) {
        move_speed = 10.0f;
      } else {
        move_speed = 5.0f;
      }

      if (input.root_layer().keyboard().is_held(Input::ScanCode::W)) {
        const auto f{m_camera.as_mat4().z};
        m_camera.translate += Math::Vec3f(f.x, f.y, f.z) * (move_speed * _delta_time);
      }
      if (input.root_layer().keyboard().is_held(Input::ScanCode::S)) {
        const auto f{m_camera.as_mat4().z};
        m_camera.translate -= Math::Vec3f(f.x, f.y, f.z) * (move_speed * _delta_time);
      }
      if (input.root_layer().keyboard().is_held(Input::ScanCode::D)) {
        const auto l{m_camera.as_mat4().x};
        m_camera.translate += Math::Vec3f(l.x, l.y, l.z) * (move_speed * _delta_time);
      }
      if (input.root_layer().keyboard().is_held(Input::ScanCode::A)) {
        const auto l{m_camera.as_mat4().x};
        m_camera.translate -= Math::Vec3f(l.x, l.y, l.z) * (move_speed * _delta_time);
      }
      if (input.root_layer().keyboard().is_released(Input::ScanCode::T)) {
        m_models.pop_back();
      }
    }

    if (input.root_layer().keyboard().is_released(Input::ScanCode::F1)) {
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

    if (input.root_layer().keyboard().is_pressed(Input::ScanCode::GRAVE)) {
      m_console.raise();
    }

    if (input.root_layer().keyboard().is_released(Input::ScanCode::ESCAPE)) {
      return false;
    }

    if (input.root_layer().keyboard().is_released(Input::ScanCode::F12)) {
      display_fullscreen->set((display_fullscreen->get() + 1) % 3);
    }

    if (input.root_layer().keyboard().is_released(Input::ScanCode::N1)) {
      m_lut_index = (m_lut_index + 1) % 35;
    }

    if (input.root_layer().keyboard().is_released(Input::ScanCode::M)) {
      static int sky = 0;
      static constexpr const char* SKYBOXES[] = {
        "base/skyboxes/nebula/nebula.json5",
        "base/skyboxes/yokohama/yokohama.json5",
        "base/skyboxes/miramar/miramar.json5"
      };
      m_skybox.load_async(SKYBOXES[sky], {512, 512});
      sky = (sky + 1) % (sizeof SKYBOXES / sizeof *SKYBOXES);
    }

    m_console.update(console);

    m_models.each_fwd([&](Render::Model& model_) {
      model_.update(_delta_time);
    });

    return true;
  }

  virtual bool on_render() {
    auto& input = engine()->input();
    auto& console = engine()->console();

    m_ibl.render(m_skybox.cubemap());

    m_color_grader.update();

    static auto display_resolution =
      console.find_variable_by_name("display.resolution")->cast<Math::Vec2i>();

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
      model_.render(m_gbuffer.target(), Math::Mat4x4f::scale({1, 1, 1}), m_camera.view(), m_camera.projection);
      model_.render_skeleton({}, &m_immediate3D);
    });

    m_indirect_lighting_pass.render(m_camera, &m_gbuffer, &m_ibl);

    // Copy the indirect result.
    m_copy_pass.render(m_indirect_lighting_pass.texture());

    // Render the skybox absolutely last into the copy pass target.
    m_skybox.render(m_copy_pass.target(), m_camera.view(), m_camera.projection,
      m_lut_index ? &m_luts[m_lut_index] : nullptr);

    // Then 3D immediates.
    m_immediate3D.render(m_copy_pass.target(), m_camera.view(), m_camera.projection);

    // Lens distortion pass.
    m_lens_distortion_pass.distortion = *lens_distortion;
    m_lens_distortion_pass.dispersion = *lens_dispersion;
    m_lens_distortion_pass.scale = *lens_scale;
    m_lens_distortion_pass.render(m_copy_pass.texture());

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

    if (*input_debug_layers) {
      input.render_regions(&m_immediate2D);
    }

    m_immediate2D.render(m_frontend.swapchain());

    return true;
  }

  void on_resize(const Math::Vec2z& _dimensions) {
    m_gbuffer.resize(_dimensions);
    m_copy_pass.attach_depth_stencil(m_gbuffer.depth_stencil());
    m_copy_pass.resize(_dimensions);
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
  Render::CopyPass m_copy_pass;
  Render::ColorGrader m_color_grader;
  Render::ColorGrader::Entry m_luts[128];
  Size m_lut_index;
  Size m_lut_count;

  Math::Camera m_camera;
};

Ptr<Application> create(Engine* _engine) {
  return make_ptr<TestGame>(Memory::SystemAllocator::instance(), _engine);
}
