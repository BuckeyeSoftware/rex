#include "rx/render/gbuffer.h"

#include "rx/render/frontend/target.h"
#include "rx/render/frontend/texture.h"
#include "rx/render/frontend/context.h"

namespace Rx::Render {

Optional<GBuffer> GBuffer::create(Frontend::Context* _frontend,
  const Options& _options)
{
  auto albedo = _frontend->create_texture2D(RX_RENDER_TAG("gbuffer albedo"));
  auto normal = _frontend->create_texture2D(RX_RENDER_TAG("gbuffer normal"));
  auto emission = _frontend->create_texture2D(RX_RENDER_TAG("gbuffer emission"));
  auto velocity = _frontend->create_texture2D(RX_RENDER_TAG("gbuffer velocity"));
  auto target = _frontend->create_target(RX_RENDER_TAG("gbuffer"));

  if (!albedo || !normal || !emission || !velocity || !target) {
    _frontend->destroy_target(RX_RENDER_TAG("gbuffer"), target);
    _frontend->destroy_texture(RX_RENDER_TAG("gbuffer velocity"), velocity);
    _frontend->destroy_texture(RX_RENDER_TAG("gbuffer emission"), emission);
    _frontend->destroy_texture(RX_RENDER_TAG("gbuffer normal"), normal);
    _frontend->destroy_texture(RX_RENDER_TAG("gbuffer albedo"), albedo);
    return nullopt;
  }

  // Albedo.
  albedo->record_format(Frontend::Texture::DataFormat::SRGBA_U8);
  albedo->record_type(Frontend::Texture::Type::ATTACHMENT);
  albedo->record_levels(1);
  albedo->record_dimensions(_options.dimensions);
  _frontend->initialize_texture(RX_RENDER_TAG("gbuffer albedo"), albedo);
  target->attach_texture(albedo, 0);

  // Normal.
  normal->record_format(Frontend::Texture::DataFormat::RGBA_U8);
  normal->record_type(Frontend::Texture::Type::ATTACHMENT);
  normal->record_levels(1);
  normal->record_dimensions(_options.dimensions);
  _frontend->initialize_texture(RX_RENDER_TAG("gbuffer normal"), normal);
  target->attach_texture(normal, 0);

  // Emission.
  emission->record_format(Frontend::Texture::DataFormat::RGBA_U8);
  emission->record_type(Frontend::Texture::Type::ATTACHMENT);
  emission->record_levels(1);
  emission->record_dimensions(_options.dimensions);
  _frontend->initialize_texture(RX_RENDER_TAG("gbuffer emission"), emission);
  target->attach_texture(emission, 0);

  // Velocity.
  velocity->record_format(Frontend::Texture::DataFormat::RG_F16);
  velocity->record_type(Frontend::Texture::Type::ATTACHMENT);
  velocity->record_levels(1);
  velocity->record_dimensions(_options.dimensions);
  _frontend->initialize_texture(RX_RENDER_TAG("gbuffer velocity"), velocity);
  target->attach_texture(velocity, 0);

  // Depth & Stencil.
  target->request_depth_stencil(Frontend::Texture::DataFormat::D24_S8,
    _options.dimensions);

  _frontend->initialize_target(RX_RENDER_TAG("gbuffer"), target);

  return GBuffer{_frontend, target, albedo, normal, emission, velocity};
}

GBuffer& GBuffer::operator=(GBuffer&& gbuffer_) {
  if (this != &gbuffer_) {
    release();
    m_frontend = Utility::exchange(gbuffer_.m_frontend, nullptr);
    m_target = Utility::exchange(gbuffer_.m_target, nullptr);
    m_albedo = Utility::exchange(gbuffer_.m_albedo, nullptr);
    m_normal = Utility::exchange(gbuffer_.m_normal, nullptr);
    m_emission = Utility::exchange(gbuffer_.m_emission, nullptr);
    m_velocity = Utility::exchange(gbuffer_.m_velocity, nullptr);
  }

  return *this;
}

void GBuffer::release() {
  if (!m_frontend) {
    return;
  }

  m_frontend->destroy_texture(RX_RENDER_TAG("gbuffer albedo"), m_albedo);
  m_frontend->destroy_texture(RX_RENDER_TAG("gbuffer normal"), m_normal);
  m_frontend->destroy_texture(RX_RENDER_TAG("gbuffer emission"), m_emission);
  m_frontend->destroy_texture(RX_RENDER_TAG("gbuffer velocity"), m_velocity);
  m_frontend->destroy_target(RX_RENDER_TAG("gbuffer"), m_target);
}

void GBuffer::clear() {
  Render::Frontend::State state;
  state.viewport.record_dimensions(m_target->dimensions());

  Render::Frontend::Buffers draw_buffers;
  draw_buffers.add(0);
  draw_buffers.add(1);
  draw_buffers.add(2);
  draw_buffers.add(3);

  m_frontend->clear(
    RX_RENDER_TAG("gbuffer"),
    state,
    m_target,
    draw_buffers,
    RX_RENDER_CLEAR_DEPTH |
    RX_RENDER_CLEAR_STENCIL |
    RX_RENDER_CLEAR_COLOR(0) |
    RX_RENDER_CLEAR_COLOR(1) |
    RX_RENDER_CLEAR_COLOR(2) |
    RX_RENDER_CLEAR_COLOR(3),
    1.0f,
    0,
    Math::Vec4f{1.0f, 1.0f, 1.0f, 1.0f}.data(),
    Math::Vec4f{1.0f, 1.0f, 1.0f, 1.0f}.data(),
    Math::Vec4f{1.0f, 1.0f, 1.0f, 1.0f}.data(),
    Math::Vec4f{0.0f, 0.0f, 0.0f, 0.0f}.data());
}

Frontend::Texture2D* GBuffer::depth_stencil() const {
  return m_target->depth_stencil();
}

} // namespace Rx::Render
