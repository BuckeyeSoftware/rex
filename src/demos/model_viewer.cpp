#include "demos/model_viewer.h"

#include "rx/engine.h"

#include "rx/render/frontend/context.h"
#include "rx/render/frontend/target.h"

#include "rx/console/variable.h"

#include "rx/math/range.h"

namespace Rx {

RX_CONSOLE_SVAR(mdl, "demo.mdl", "model", "base/models/helmet/helmet.json5");
RX_CONSOLE_SVAR(sky, "demo.sky", "skybox", "base/skyboxes/sky_cloudy/sky_cloudy.json5");

ModelViewer::ModelViewer(Engine* _engine)
  : Application{_engine}
{
}

bool ModelViewer::on_init() {
  // Start with camera 1 unit back on Z axis.
  m_camera.translate = {0.0f, 0.0f, -1.0f};

  auto renderer = engine()->renderer();
  auto swapchain = renderer->swapchain();

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

  // Renderables.
  m_model = Render::Model::create(renderer);
  if (!m_model || !m_model->load(*mdl)) {
    return false;
  }

  m_skybox = Render::Skybox::create(renderer);
  if (!m_skybox || !m_skybox->load(*sky, {1024, 1024})) {
    return false;
  }

  return true;
}

bool ModelViewer::on_update(Float32 _delta_time) {
  auto& input = engine()->input();
  const auto& renderer = engine()->renderer();

  // Terminate when ESCAPE is pressed.
  if (input.root_layer().keyboard().is_released(Input::ScanCode::ESCAPE)) {
    return false;
  }

  // GRAVE to toggle mouse capture.
  if (input.root_layer().keyboard().is_released(Input::ScanCode::GRAVE)) {
    input.root_layer().capture_mouse(!input.root_layer().is_mouse_captured());
  }

  // Update camera projection.
  const auto& dimensions = renderer->swapchain()->dimensions().cast<Float32>();
  m_camera.projection = Math::perspective(90.0f, {0.01f, 2048.0f},
    dimensions.w / dimensions.h);

  m_model->update(_delta_time);

  // Rotate on the Y axis slowly.
  m_transform.rotation *=
    Math::Quatf({0.0f, 1.0f, 0.0f}, -0.5f * _delta_time);

  return true;
}

bool ModelViewer::on_render() {
  auto renderer = engine()->renderer();
  auto swapchain = renderer->swapchain();

  // Update image based lighting with HDRI or CM of Skybox.
  m_image_based_lighting->render(m_skybox->texture());

  // Clear the geometry buffer.
  m_gbuffer->clear();

  // Render the model into the geometry buffer.
  m_model->render(
    m_gbuffer->target(),
    m_transform.as_mat4(),
    m_camera.view(),
    m_camera.projection,
    0);

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

  return true;
}

void ModelViewer::on_resize(const Math::Vec2z& _dimensions) {
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
  return make_ptr<Rx::ModelViewer>(Rx::Memory::SystemAllocator::instance(), _engine);
}*/