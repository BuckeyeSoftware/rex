#include "demos/model_banner.h"

#include "rx/engine.h"

#include "rx/render/frontend/context.h"
#include "rx/render/frontend/target.h"

#include "rx/core/concurrency/thread_pool.h"

#include "rx/console/variable.h"

#include "rx/math/range.h"

#include <stdio.h>

namespace Rx {

static constexpr const char* SKYBOX_PATH = "base/skyboxes/yokohama/yokohama.json5";
static constexpr const char* MODEL_PATHS[] = {
  "base/models/food_pineapple/food_pineapple.json5",
  "base/models/drak_chest/drak_chest.json5",
  "base/models/modern_nightstand/modern_nightstand.json5",
  "base/models/raspberry_pico/raspberry_pico.json5",
  "base/models/ratcher_house/ratcher_house.json5"
};

static constexpr const auto SPACING_BETWEEN_MODELS = 2.0f;
static constexpr const auto N_MODELS = sizeof MODEL_PATHS / sizeof *MODEL_PATHS;
static constexpr const auto N_REPEAT = 1024;
static constexpr const auto N_MAX = N_REPEAT * 2 * N_MODELS + N_MODELS;

RX_CONSOLE_IVAR(selection, "selection", "model selection", -INT_MAX, INT_MAX, 0);

static void wrap_selection() {
  if (*selection < 0 || *selection >= Sint32(N_MAX)) {
    selection->set(N_REPEAT * N_MODELS, false);
  }
}

ModelBanner::ModelBanner(Engine* _engine)
  : Application{_engine}
{
}

bool ModelBanner::on_init() {
  m_camera.translate = {0.0f, 0.0f, -1.0f};

  wrap_selection();

  auto on_selection_change =
    selection->on_change([this](Console::Variable<Rx::Sint32>& _var) {
      wrap_selection();
      m_animation = Animation{m_camera.translate.x, m_transforms[_var].translate.x, 1.0f};
    });

  if (!on_selection_change) {
    return false;
  }

  m_on_selection_change = Utility::move(*on_selection_change);

  auto renderer = engine()->renderer();
  auto& input = engine()->input();
  auto swapchain = renderer->swapchain();

  input.root_layer().capture_mouse(false);

  {
    Render::GBuffer::Options options;
    options.dimensions = swapchain->dimensions();

    m_gbuffer = Render::GBuffer::create(renderer, options);
    if (!m_gbuffer) {
      return false;
    }
  }

  {
    Render::ImageBasedLighting::Options options;
    options.irradiance_size = 16;
    options.prefilter_size = 64;

    m_image_based_lighting = Render::ImageBasedLighting::create(renderer, options);
    if (!m_image_based_lighting) {
      return false;
    }
  }

  {
    Render::IndirectLightingPass::Options options;
    options.stencil = m_gbuffer->depth_stencil();
    options.dimensions = swapchain->dimensions();

    m_indirect_lighting_pass = Render::IndirectLightingPass::create(renderer, options);
    if (!m_indirect_lighting_pass) {
      return false;
    }
  }

  {
    Render::CopyPass::Options options;
    options.dimensions = swapchain->dimensions();
    options.format = Render::Frontend::Texture::DataFormat::R_F16;

    m_copy_pass = Render::CopyPass::create(renderer, options);
    if (!m_copy_pass) {
      return false;
    }
  }

  // Load the skybox.
  m_skybox = Render::Skybox::create(renderer);
  if (!m_skybox || !m_skybox->load(SKYBOX_PATH, {1024, 1024})) {
    return false;
  }

  // Load some models.
  for (Size i = 0; i < N_MODELS; i++) {
    auto model = Render::Model::create(renderer);
    if (!model || !model->load(engine()->thread_pool(), MODEL_PATHS[i])) {
      return false;
    }

    if (!m_models.push_back(Utility::move(*model))) {
      return false;
    }
  }

  m_immediate_2d = Render::Immediate2D::create(renderer);
  if (!m_immediate_2d) {
    return false;
  }

  // Start any animations.
  m_models.each_fwd([](Render::Model& model_) {
    model_.animate(0, true);
  });

  Math::Vec3f translate;
  translate.z = 0.75f;

  auto add_draw_models = [&] {
    for (Size i = 0; i < N_MODELS; i++) {
      Math::Transform transform;
      transform.translate = translate;
      if (!m_draw_models.push_back(&m_models[i])) {
        return false;
      }
      if (!m_transforms.push_back(transform)) {
        return false;
      }
      translate.x += SPACING_BETWEEN_MODELS;
    }
    return true;
  };

  // Put N_REPEAT instances of our N_MODELS down.
  for (Size i = 0; i < N_REPEAT; i++) {
    if (!add_draw_models()) {
      return false;
    }
  }

  // Make the middle our starting selection.
  selection->set(m_draw_models.size(), false);

  if (!add_draw_models()) {
    return false;
  }

  for (Size i = 0; i < N_REPEAT; i++) {
    if (!add_draw_models()) {
      return false;
    }
  }

  // Set the camera location to the first by default, note this is at index 1
  // since index 0 contains the last model.
  m_camera.translate.x = m_transforms[*selection].translate.x;

  return true;
}

bool ModelBanner::on_update(Float32 _delta_time) {
  auto& input = engine()->input();
  const auto& renderer = engine()->renderer();

#if !defined(RX_PLATFORM_EMSCRIPTEN)
  // Terminate when ESCAPE is pressed.
  if (input.root_layer().keyboard().is_released(Input::ScanCode::ESCAPE)) {
    return false;
  }

  // GRAVE to toggle mouse capture.
  if (input.root_layer().keyboard().is_released(Input::ScanCode::GRAVE)) {
    input.root_layer().capture_mouse(!input.root_layer().is_mouse_captured());
  }

  // To test selection.
  if (input.root_layer().keyboard().is_released(Input::ScanCode::A)) {
    selection->set(selection->get() - 1);
  } else if (input.root_layer().keyboard().is_released(Input::ScanCode::D)) {
    selection->set(selection->get() + 1);
  }
#endif

  if (input.root_layer().mouse().is_held(1)) {
    const auto& delta = input.root_layer().mouse().movement().cast<Float32>();

    const auto rx = Math::Quatf({1.0f, 0.0f, 0.0f}, delta.y * _delta_time);
    const auto ry = Math::Quatf({0.0f, 1.0f, 0.0f}, delta.x * _delta_time);

    m_transforms[(*selection % N_MODELS)].rotation *= rx * ry;
  }

  // Update camera projection.
  const auto& dimensions = renderer->swapchain()->dimensions().cast<Float32>();
  m_camera.projection = Math::perspective(45.0f, {0.01f, 1024.0f},
    dimensions.w / dimensions.h);

  m_models.each_fwd([&](Render::Model& _model) {
    _model.update(_delta_time);
  });

  if (m_animation) {
    m_animation->update(_delta_time);
  }

  return true;
}

bool ModelBanner::on_render() {
  auto renderer = engine()->renderer();
  auto swapchain = renderer->swapchain();

  // Update image based lighting with HDRI or CM of Skybox.
  m_image_based_lighting->render(m_skybox->texture());

  // Clear the geometry buffer.
  m_gbuffer->clear();

  // Render the models into the geometry buffer.
  for (Size i = *selection - N_MODELS; i < *selection + N_MODELS; i++) {
    // Share rotation of instances
    Math::Transform transform;
    transform.translate = m_transforms[i].translate;
    transform.rotation = m_transforms[(i % N_MODELS)].rotation;
    m_draw_models[i]->render(
      m_gbuffer->target(),
      transform.as_mat4(),
      m_camera.view(),
      m_camera.projection,
      0);
  }

  // Copy the depth from the geometry buffer as it's sampled during the indirect
  // lighting pass yet the indirect lighting pass has the geometry buffer's
  // depth stencil as an attachment to perform stencil testing. This forms a
  // feedback loop which is undefined behavior.
  m_copy_pass->render(m_gbuffer->depth_stencil());

  // Render indirect lighting pass now.
  Render::IndirectLightingPass::Input input;
  input.albedo     = m_gbuffer->albedo();
  input.normal     = m_gbuffer->normal();
  input.emission   = m_gbuffer->emission();
  input.depth      = m_copy_pass->texture();
  input.irradiance = m_image_based_lighting->irradiance_map();
  input.prefilter  = m_image_based_lighting->prefilter();
  input.scale_bias = m_image_based_lighting->scale_bias();
  m_indirect_lighting_pass->render(m_camera, input);

  // Clear and blit the swapchain.
  Render::Frontend::State state;
  state.viewport.record_dimensions(swapchain->dimensions());

  Render::Frontend::Buffers buffers;
  buffers.add(0);

  renderer->clear(
    RX_RENDER_TAG("swapchain clear"),
    state,
    swapchain,
    buffers,
    RX_RENDER_CLEAR_COLOR(0),
    Math::Vec4f{0.0f, 0.0f, 0.0f, 0.0f}.data());

  // Enable blending for the blit.
  state.blend.record_enable(true);
  state.blend.record_blend_factors(
    Render::Frontend::BlendState::FactorType::SRC_ALPHA,
    Render::Frontend::BlendState::FactorType::ONE_MINUS_SRC_ALPHA);

  // Blit the contents of the indirect lighting pass directly onto the swapchain.
  renderer->blit(
    RX_RENDER_TAG("swapchain blit"),
    state,
    m_indirect_lighting_pass->target(),
    0,
    swapchain,
    0);

  const Math::Vec4f gradient_l[4] = {
    {0.0f, 0.0f, 0.0f, 1.0f},
    {0.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 1.0f},
  };

  const Math::Vec4f gradient_r[4] = {
    {0.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 1.0f},
    {0.0f, 0.0f, 0.0f, 1.0f},
    {0.0f, 0.0f, 0.0f, 0.0f},
  };

  const auto& dimensions = swapchain->dimensions().cast<Float32>();
  m_immediate_2d->frame_queue().record_rectangle_gradient(
    {0, 0},
    dimensions * Math::Vec2f{0.25f, 1.0f},
    0.0f,
    gradient_l);

  m_immediate_2d->frame_queue().record_rectangle_gradient(
    {dimensions.w - dimensions.w * 0.25f, 0},
    dimensions * Math::Vec2f{0.25f, 1.0f},
    0.0f,
    gradient_r);


  m_immediate_2d->render(swapchain);

  return true;
}

void ModelBanner::on_resize(const Math::Vec2z& _dimensions) {
  // Recreate the geometry buffer.
  {
    Render::GBuffer::Options options;
    options.dimensions = _dimensions;
    m_gbuffer->recreate(options);
  }

  // Recreate the copy pass.
  {
    Render::CopyPass::Options options;
    options.format = Render::Frontend::Texture::DataFormat::R_F16;
    options.dimensions = _dimensions;
    m_copy_pass->recreate(options);
  }

  // Recreate the indirect lighting pass.
  {
    Render::IndirectLightingPass::Options options;
    options.dimensions = _dimensions;
    options.stencil = m_gbuffer->depth_stencil();
    m_indirect_lighting_pass->recreate(options);
  }
}

} // namespace Rx

/*
Rx::Ptr<Rx::Application> create(Rx::Engine* _engine) {
  return make_ptr<Rx::ModelBanner>(Rx::Memory::SystemAllocator::instance(), _engine);
}*/