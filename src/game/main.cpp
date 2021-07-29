#include <stdio.h>
#include <stdlib.h>

#include "rx/application.h"
#include "rx/engine.h"

#include "rx/render/frontend/context.h"
#include "rx/render/frontend/target.h"
#include "rx/render/frontend/downloader.h"
#include "rx/render/frontend/sampler.h"

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
#include "rx/render/particle_system.h"

#include "rx/hud/console.h"
#include "rx/hud/frame_graph.h"
#include "rx/hud/memory_stats.h"
#include "rx/hud/render_stats.h"

#include "rx/console/context.h"
#include "rx/console/variable.h"

#include "rx/math/camera.h"
#include "rx/math/range.h"

#include "rx/core/filesystem/directory.h"
#include "rx/core/filesystem/unbuffered_file.h"

#include "rx/core/concurrency/thread_pool.h"

#include "rx/particle/assembler.h"
#include "rx/particle/system.h"
#include "rx/particle/emitter.h"

#include "rx/core/math/sin.h"
#include "rx/core/math/cos.h"
#include "rx/core/math/constants.h"

using namespace Rx;

static constexpr const char* LUTS[] = {
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
    , m_console{&m_immediate2D, engine()->input()}
    , m_frame_graph{&m_immediate2D}
    , m_memory_stats{&m_immediate2D}
    , m_render_stats{&m_immediate2D}
    , m_models{m_frontend.allocator()}
    , m_color_grader{&m_frontend}
    , m_lut_index{0}
    , m_lut_count{0}
    , m_particle_system{m_frontend.allocator()}
  {
    engine()->input().root_layer().raise();
  }

  ~TestGame() {
  }

  Vector<Math::Transform> m_mdl_tranforms;
  Vector<Math::Vec3f> m_mdl_rotations;

  virtual bool on_init() {
    m_camera.translate = {0.0f, 0.0f, -0.5f};

    const auto& dimensions = m_frontend.swapchain()->dimensions();

    if (!m_particle_system.resize(500'000, 128)) {
      return false;
    }

    Particle::Assembler assembler{m_frontend.allocator()};
    m_particle_program = assembler.assemble("base/particles/sphere.asm");
    if (!m_particle_program) {
      return false;
    }

    if (auto renderable = Render::ParticleSystem::create(&m_frontend)) {
      m_particle_system_render = Utility::move(*renderable);
    } else {
      return false;
    }

    if (auto renderable = Render::Immediate3D::create(&m_frontend)) {
      m_immediate3D = Utility::move(*renderable);
    } else {
      return false;
    }

    if (auto renderable = Render::Immediate2D::create(&m_frontend)) {
      m_immediate2D = Utility::move(*renderable);
    } else {
      return false;
    }

    {
      Render::GBuffer::Options options;
      options.dimensions = dimensions;
      if (auto gbuffer = Render::GBuffer::create(&m_frontend, options)) {
        m_gbuffer = Utility::move(*gbuffer);
      } else {
        return false;
      }
    }

    if (auto skybox = Render::Skybox::create(&m_frontend)) {
      m_skybox = Utility::move(*skybox);
      if (!m_skybox.load("base/skyboxes/yokohama/yokohama.json5", {1024, 1024})) {
        return false;
      }
    } else {
      return false;
    }

    if (auto pass = Render::LensDistortionPass::create(&m_frontend, dimensions)) {
      m_lens_distortion_pass = Utility::move(*pass);
    } else {
      return false;
    }

    {
      Render::IndirectLightingPass::Options options;
      options.stencil = m_gbuffer.depth_stencil();
      options.dimensions = dimensions;

      if (auto pass = Render::IndirectLightingPass::create(&m_frontend, options)) {
        m_indirect_lighting_pass = Utility::move(*pass);
      } else {
        return false;
      }
    }

    {
      Render::CopyPass::Options options;
      options.format = Render::Frontend::Texture::DataFormat::R_F16;
      options.dimensions = dimensions;

      if (auto pass = Render::CopyPass::create(&m_frontend, options)) {
        m_depth_copy_pass = Utility::move(*pass);
      } else {
        return false;
      }
    }

    Render::ImageBasedLighting::Options options;
    options.irradiance_size = 16;
    options.prefilter_size = 128;
    if (auto ibl = Render::ImageBasedLighting::create(&m_frontend, options)) {
      m_ibl = Utility::move(*ibl);
    } else {
      return false;
    }

    // Make index 0 a neutral LUT.
    if (auto neutral = m_color_grader.allocate(32)) {
      m_luts[m_lut_count++] = Utility::move(*neutral);
    }
    
    // Load in all the grading LUTs and update the atlas.
    for (Size i = 0; i < sizeof LUTS / sizeof* LUTS; i++) {
      if (auto lut = m_color_grader.load(LUTS[i])) {
        m_luts[m_lut_count++] = Utility::move(*lut);
      }
    }

    m_color_grader.update();

    Math::Transform transform;
    // transform.translate.x = -((8.0f * (7.0f - 1.0f)) * 0.5f);

    auto add_model = [&](const char* _file_name) {
      if (auto model = Render::Model::create(&m_frontend)) {
        if (model->load(engine()->thread_pool(), _file_name)) {
          if (m_models.push_back(Utility::move(*model))) {
            if (m_mdl_tranforms.push_back(transform)) {
              m_mdl_rotations.emplace_back(0.0f, 0.0f, 0.0f);
              transform.translate.x += 5.0f;
              return true;
            }
          }
        }
      }
      return false;
    };

    if (!add_model("base/models/bedside_wardrobe/bedside_wardrobe.json5")) return false;
    if (!add_model("base/models/drak_chest/drak_chest.json5")) return false;
    if (!add_model("base/models/food_pineapple/food_pineapple.json5")) return false;
    if (!add_model("base/models/modern_nightstand/modern_nightstand.json5")) return false;
    if (!add_model("base/models/osiris_monitor/osiris_monitor.json5")) return false;
    if (!add_model("base/models/raspberry_pico/raspberry_pico.json5")) return false;
    if (!add_model("base/models/ratcher_house/ratcher_house.json5")) return false;
    if (!add_model("base/models/spinal_roach/spinal_roach.json5")) return false;

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
    m_camera.projection = Math::perspective(90.0f, {0.01f, 2048.0f},
      dimensions.w / dimensions.h);

    if (input.root_layer().is_active()) {
      Float32 move_speed{0.0f};
      const Float32 sens{0.2f};
      const auto &delta = input.root_layer().mouse().movement();

      Math::Vec3f move{static_cast<Float32>(delta.y) * sens, static_cast<Float32>(delta.x) * sens, 0.0f};
      m_mouse += move;

      m_camera.rotation = Math::Mat3x3f::rotate(m_mouse);

      if (input.root_layer().keyboard().is_held(Input::ScanCode::LEFT_CONTROL)) {
        move_speed = 25.0f;
      } else {
        move_speed = 10.0f;
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
      if (input.root_layer().keyboard().is_released(Input::ScanCode::Y)) {
        static int group = 0;
        // for (int i = 0; i <= 100; i++) {
          auto index = m_particle_system.add_emitter(group, *m_particle_program, 1000.0f);
          if (index) {
            // auto angle = 2.0f * Math::PI<Float32> * i / 100.0f;
            // auto radius = 10.0f;
            auto& emitter = m_particle_system.emitter(*index);
            emitter[0] = 10.0f;

            // emitter[0] = Math::cos(angle) * radius + m_camera.translate.x;
            // emitter[1] = m_camera.translate.y;
            // emitter[2] = Math::sin(angle) * radius + m_camera.translate.z;
            // emitter[3] = 0.0f;
          }
        // }
        group++;
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
      m_lut_index = (m_lut_index + 1) % (sizeof LUTS / sizeof *LUTS);
      engine()->console().print("LUT: %s", m_lut_index ? LUTS[m_lut_index - 1] : "NEUTRAL");
    }

    if (input.root_layer().keyboard().is_released(Input::ScanCode::M)) {
      static int sky = 0;
      static constexpr const char* SKYBOXES[] = {
        "base/skyboxes/nebula/nebula.json5",
        "base/skyboxes/yokohama/yokohama.json5",
        "base/skyboxes/miramar/miramar.json5"
      };
      m_skybox.load_async(engine()->thread_pool(), SKYBOXES[sky], {512, 512});
      sky = (sky + 1) % (sizeof SKYBOXES / sizeof *SKYBOXES);
    }

    m_console.update(console);

    m_models.each_fwd([&](Render::Model& model_) {
      model_.update(_delta_time);
    });

    m_mdl_rotations.each_fwd([&](Math::Vec3f& rotation_) {
      // rotation_.y += 25.0f * _delta_time;
    });

    m_particle_system.update(_delta_time);

    return true;
  }

  virtual bool on_render() {
    auto& input = engine()->input();

    // When calculating
    m_ibl.render(m_skybox.texture());

    m_color_grader.update();

    m_gbuffer.clear();

    const auto n_models = m_models.size();
    for (Size i = 0; i < n_models; i++) {
      auto& model = m_models[i];
      auto& transform = m_mdl_tranforms[i];
      const auto& rotate = m_mdl_rotations[i];
      transform.rotation = Math::Mat3x3f::rotate(rotate);
      model.render(m_gbuffer.target(), transform.as_mat4(), m_camera.view(),
        m_camera.projection,
        Render::Model::SKELETON | Render::Model::BOUNDS,
        &m_immediate3D);
    }

    // Copy the depth for IndirectLightingPass because DS.
    Render::Frontend::Sampler sampler;
    sampler.record_min_filter(Render::Frontend::Sampler::Filter::LINEAR);
    sampler.record_mag_filter(Render::Frontend::Sampler::Filter::LINEAR);
    m_depth_copy_pass.render(m_gbuffer.depth_stencil(), sampler);

    // Render indirect lighting pass.
    Render::IndirectLightingPass::Input ilp_input;
    ilp_input.albedo = m_gbuffer.albedo();
    ilp_input.normal = m_gbuffer.normal();
    ilp_input.emission = m_gbuffer.emission();
    ilp_input.depth = m_depth_copy_pass.texture();
    ilp_input.irradiance = m_ibl.irradiance_map();
    ilp_input.prefilter = m_ibl.prefilter();
    ilp_input.scale_bias = m_ibl.scale_bias();
    m_indirect_lighting_pass.render(m_camera, ilp_input);

    // Render the skybox absolutely last into that target.
    m_skybox.render(m_indirect_lighting_pass.target(), m_camera.view(),
      m_camera.projection, &m_luts[m_lut_index]);

    m_particle_system.groups().each_fwd([&](const Particle::System::Group& _group) {
      m_immediate3D.frame_queue().record_wire_box(
        {1.0f, 0.0f, 0.0f, 1.0f},
        _group.bounds,
        0);
    });

    // Then 3D immediates.
    m_immediate3D.render(m_indirect_lighting_pass.target(), m_camera.view(), m_camera.projection);

    m_particle_system_render.render(&m_particle_system,
      m_indirect_lighting_pass.target(), {}, m_camera.view(), m_camera.projection);

    // Blit.
    Render::Frontend::State state;
    state.viewport.record_dimensions(m_frontend.swapchain()->dimensions());

    state.blend.record_enable(true);
    state.blend.record_blend_factors(
      Render::Frontend::BlendState::FactorType::SRC_ALPHA,
      Render::Frontend::BlendState::FactorType::ONE_MINUS_SRC_ALPHA);

    Render::Frontend::Buffers buffers;
    buffers.add(0);

    m_frontend.clear(
      RX_RENDER_TAG("swapchain clear"),
      state,
      m_frontend.swapchain(),
      buffers,
      RX_RENDER_CLEAR_COLOR(0),
      Math::Vec4f{0.0f, 0.0f, 0.0f, 0.0f}.data());

    m_frontend.blit(
      RX_RENDER_TAG("test"),
      state,
      m_indirect_lighting_pass.target(),
      0,
      m_frontend.swapchain(),
      0);

    m_frame_graph.render();
    m_memory_stats.render();

    m_console.render();

    m_render_stats.render();

    if (*input_debug_layers) {
      input.render_regions(&m_immediate2D);
    }

    m_immediate2D.render(m_frontend.swapchain());

    return true;
  }

  void on_resize(const Math::Vec2z& _dimensions) {
    {
      Render::GBuffer::Options options;
      options.dimensions = _dimensions;
      m_gbuffer.recreate(options);
    }

    {
      Render::CopyPass::Options options;
      options.format = Render::Frontend::Texture2D::DataFormat::R_F16;
      options.dimensions = _dimensions;

      m_depth_copy_pass.recreate(options);
    }

    {
      Render::IndirectLightingPass::Options options;
      options.dimensions = _dimensions;
      options.stencil = m_gbuffer.depth_stencil();
      m_indirect_lighting_pass.recreate(options);
    }
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
  Math::Transform m_transform;

  Render::ImageBasedLighting m_ibl;

  Render::IndirectLightingPass m_indirect_lighting_pass;
  Render::LensDistortionPass m_lens_distortion_pass;
  Render::CopyPass m_depth_copy_pass;
  Render::ColorGrader m_color_grader;
  Render::ColorGrader::Entry m_luts[128];
  Size m_lut_index;
  Size m_lut_count;

  Render::ParticleSystem m_particle_system_render;
  Particle::System m_particle_system;
  Optional<Particle::Program> m_particle_program;

  Math::Camera m_camera;
  Math::Vec3f m_mouse;
};


Ptr<Application> create(Engine* _engine) {
  return make_ptr<TestGame>(Memory::SystemAllocator::instance(), _engine);
}
