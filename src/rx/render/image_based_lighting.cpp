#include <stddef.h> // offsetof

#include "rx/render/image_based_lighting.h"
#include "rx/render/skybox.h"

#include "rx/core/algorithm/max.h"

#include "rx/render/frontend/target.h"
#include "rx/render/frontend/buffer.h"
#include "rx/render/frontend/texture.h"
#include "rx/render/frontend/context.h"
#include "rx/render/frontend/technique.h"

namespace Rx::Render {

ImageBasedLighting::ImageBasedLighting(ImageBasedLighting&& ibl_)
  : m_frontend{Utility::exchange(ibl_.m_frontend, nullptr)}
  , m_irradiance_texture{Utility::exchange(ibl_.m_irradiance_texture, nullptr)}
  , m_prefilter_texture{Utility::exchange(ibl_.m_prefilter_texture, nullptr)}
  , m_scale_bias_texture{Utility::exchange(ibl_.m_scale_bias_texture, nullptr)}
{
}

Optional<ImageBasedLighting> ImageBasedLighting::create(Frontend::Context* _frontend) {
  auto scale_bias_technique =
    _frontend->find_technique_by_name("brdf_integration");

  if (!scale_bias_technique) {
    return nullopt;
  }

  // Create scale & bias texture.
  auto texture = _frontend->create_texture2D(RX_RENDER_TAG("scale / bias"));
  if (!texture) {
    return nullopt;
  }

  texture->record_format(Frontend::Texture::DataFormat::RGBA_U8);
  texture->record_type(Frontend::Texture::Type::ATTACHMENT);
  texture->record_levels(1);
  texture->record_dimensions({256, 256});
  texture->record_filter({true, false, false});
  texture->record_wrap({Frontend::Texture::WrapType::CLAMP_TO_EDGE,
                        Frontend::Texture::WrapType::CLAMP_TO_EDGE});
  _frontend->initialize_texture(RX_RENDER_TAG("scale / bias"), texture);

  // Create render target.
  auto target = _frontend->create_target(RX_RENDER_TAG("scale / bias"));
  if (!target) {
    _frontend->destroy_texture(RX_RENDER_TAG("scale / bias"), texture);
    return nullopt;
  }

  target->attach_texture(texture, 0);
  _frontend->initialize_target(RX_RENDER_TAG("scale / bias"), target);

  // Render the scale bias texture.
  Frontend::State state;
  state.viewport.record_dimensions(target->dimensions());
  state.cull.record_enable(false);

  Frontend::Buffers draw_buffers;
  draw_buffers.add(0);

  _frontend->draw(
    RX_RENDER_TAG("ibl: scale bias"),
    state,
    target,
    draw_buffers,
    nullptr,
    *scale_bias_technique,
    3,
    0,
    0,
    0,
    0,
    Frontend::PrimitiveType::TRIANGLES,
    {});

  _frontend->destroy_target(RX_RENDER_TAG("scale / bias"), target);

  ImageBasedLighting ibl;
  ibl.m_frontend = _frontend;
  ibl.m_irradiance_texture = nullptr;
  ibl.m_prefilter_texture = nullptr;
  ibl.m_scale_bias_texture = texture;

  return ibl;
}

void ImageBasedLighting::release() {
  if (m_frontend) {
    m_frontend->destroy_texture(RX_RENDER_TAG("ibl: irradiance"), m_irradiance_texture);
    m_frontend->destroy_texture(RX_RENDER_TAG("ibl: prefilter"), m_prefilter_texture);
    m_frontend->destroy_texture(RX_RENDER_TAG("ibl: scale bias"), m_scale_bias_texture);
  }
}

void ImageBasedLighting::render(Frontend::TextureCM* _environment, Size _irradiance_map_size, ColorGrader::Entry* _grading) {
  // NOTE(dweiler): Artifically limit the maximum size of IRM to avoid TDR.
  _irradiance_map_size = Algorithm::min(_irradiance_map_size, 32_z);

  Frontend::Technique* irradiance_technique = m_frontend->find_technique_by_name("irradiance_map");
  Frontend::Technique* prefilter_technique = m_frontend->find_technique_by_name("prefilter_environment_map");

  // Destroy the old textures irradiance and prefilter textures and create new ones.
  m_frontend->destroy_texture(RX_RENDER_TAG("ibl: irradiance"), m_irradiance_texture);
  m_frontend->destroy_texture(RX_RENDER_TAG("ibl: prefilter"), m_prefilter_texture);

  m_irradiance_texture = m_frontend->create_textureCM(RX_RENDER_TAG("ibl: irradiance"));
  m_irradiance_texture->record_format(Frontend::Texture::DataFormat::RGBA_U8);
  m_irradiance_texture->record_type(Frontend::Texture::Type::ATTACHMENT);
  m_irradiance_texture->record_levels(1);
  m_irradiance_texture->record_dimensions({_irradiance_map_size, _irradiance_map_size});
  m_irradiance_texture->record_filter({true, false, false});
  m_irradiance_texture->record_wrap({Frontend::Texture::WrapType::CLAMP_TO_EDGE,
                                     Frontend::Texture::WrapType::CLAMP_TO_EDGE,
                                     Frontend::Texture::WrapType::CLAMP_TO_EDGE});
  m_frontend->initialize_texture(RX_RENDER_TAG("ibl: irradiance"), m_irradiance_texture);

  static constexpr const auto MAX_PREFILTER_LEVELS = 5_z;
  m_prefilter_texture = m_frontend->create_textureCM(RX_RENDER_TAG("ibl: prefilter"));
  m_prefilter_texture->record_format(Frontend::Texture::DataFormat::RGBA_U8);
  m_prefilter_texture->record_type(Frontend::Texture::Type::ATTACHMENT);
  m_prefilter_texture->record_levels(MAX_PREFILTER_LEVELS + 1);
  m_prefilter_texture->record_dimensions(_environment->dimensions());
  m_prefilter_texture->record_filter({true, false, true});
  m_prefilter_texture->record_wrap({Frontend::Texture::WrapType::CLAMP_TO_EDGE,
                                    Frontend::Texture::WrapType::CLAMP_TO_EDGE,
                                    Frontend::Texture::WrapType::CLAMP_TO_EDGE});
  m_frontend->initialize_texture(RX_RENDER_TAG("irradiance map"), m_prefilter_texture);

  // Render irradiance map.
  {
    Frontend::Target* target{m_frontend->create_target(RX_RENDER_TAG("ibl: irradiance"))};
    target->attach_texture(m_irradiance_texture, 0);
    m_frontend->initialize_target(RX_RENDER_TAG("ibl: irradiance"), target);

    Frontend::Textures draw_textures;
    Frontend::Program* program = irradiance_technique->permute(_grading ? 1 << 0 : 0);
    program->uniforms()[0].record_sampler(draw_textures.add(_environment));
    program->uniforms()[1].record_int(static_cast<int>(_irradiance_map_size));
    if (_grading) {
      program->uniforms()[2].record_sampler(draw_textures.add(_grading->atlas()->texture()));
      program->uniforms()[3].record_vec2f(_grading->properties());
    }

    Frontend::State state;
    state.viewport.record_dimensions(target->dimensions());
    state.cull.record_enable(false);

    Frontend::Buffers draw_buffers;
    draw_buffers.add(0);
    draw_buffers.add(1);
    draw_buffers.add(2);
    draw_buffers.add(3);
    draw_buffers.add(4);
    draw_buffers.add(5);

    m_frontend->draw(
      RX_RENDER_TAG("irradiance map"),
      state,
      target,
      draw_buffers,
      nullptr,
      program,
      3,
      0,
      0,
      0,
      0,
      Frontend::PrimitiveType::TRIANGLES,
      draw_textures);

    m_frontend->destroy_target(RX_RENDER_TAG("ibl: irradiance"), target);
  }

  // Render prefilter environment map.
  {
    Frontend::Program* program = prefilter_technique->permute(_grading ? 1 << 0 : 0);

    for (Size i{0}; i <= MAX_PREFILTER_LEVELS; i++) {
      Frontend::Target* target{m_frontend->create_target(RX_RENDER_TAG("ibl: prefilter"))};
      target->attach_texture(m_prefilter_texture, i);
      m_frontend->initialize_target(RX_RENDER_TAG("ibl: prefilter"), target);

      Frontend::Textures draw_textures;
      draw_textures.add(_environment);

      program->uniforms()[0].record_sampler(draw_textures.add(_environment));
      const Float32 roughness{static_cast<Float32>(i) / MAX_PREFILTER_LEVELS};
      program->uniforms()[1].record_float(roughness);
      if (_grading) {
        program->uniforms()[2].record_sampler(draw_textures.add(_grading->atlas()->texture()));
        program->uniforms()[3].record_vec2f(_grading->properties());
      }

      Frontend::State state;
      state.viewport.record_dimensions(target->dimensions());
      state.cull.record_enable(false);

      Frontend::Buffers draw_buffers;
      draw_buffers.add(0);
      draw_buffers.add(1);
      draw_buffers.add(2);
      draw_buffers.add(3);
      draw_buffers.add(4);
      draw_buffers.add(5);

      m_frontend->draw(
        RX_RENDER_TAG("ibl: prefilter"),
        state,
        target,
        draw_buffers,
        nullptr,
        program,
        3,
        0,
        0,
        0,
        0,
        Frontend::PrimitiveType::TRIANGLES,
        draw_textures);

      m_frontend->destroy_target(RX_RENDER_TAG("ibl: prefilter"), target);
    }
  }
}

} // namespace Rx::Render
