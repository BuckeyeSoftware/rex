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

ImageBasedLighting::ImageBasedLighting(Frontend::Context* _frontend)
  : m_frontend{_frontend}
  , m_irradiance_texture{nullptr}
  , m_prefilter_texture{nullptr}
  , m_scale_bias_texture{nullptr}
{
  m_scale_bias_texture = m_frontend->create_texture2D(RX_RENDER_TAG("ibl: scale bias"));
  m_scale_bias_texture->record_format(Frontend::Texture::DataFormat::k_rgba_u8);
  m_scale_bias_texture->record_type(Frontend::Texture::Type::ATTACHMENT);
  m_scale_bias_texture->record_levels(1);
  m_scale_bias_texture->record_dimensions({256, 256});
  m_scale_bias_texture->record_filter({true, false, false});
  m_scale_bias_texture->record_wrap({
                                            Frontend::Texture::WrapType::k_clamp_to_edge,
                                            Frontend::Texture::WrapType::k_clamp_to_edge});
  m_frontend->initialize_texture(RX_RENDER_TAG("ibl: scale bias"), m_scale_bias_texture);

  // Render scale bias texture
  Frontend::Technique* scale_bias_technique{m_frontend->find_technique_by_name("brdf_integration")};
  Frontend::Target* target{m_frontend->create_target(RX_RENDER_TAG("ibl: scale bias"))};
  target->attach_texture(m_scale_bias_texture, 0);
  m_frontend->initialize_target(RX_RENDER_TAG("ibl: scale bias"), target);

  Frontend::State state;
  state.viewport.record_dimensions(target->dimensions());
  state.cull.record_enable(false);

  Frontend::Buffers draw_buffers;
  draw_buffers.add(0);

  m_frontend->draw(
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

  m_frontend->destroy_target(RX_RENDER_TAG("ibl: scale bias"), target);
}

ImageBasedLighting::~ImageBasedLighting() {
  m_frontend->destroy_texture(RX_RENDER_TAG("ibl: irradiance"), m_irradiance_texture);
  m_frontend->destroy_texture(RX_RENDER_TAG("ibl: prefilter"), m_prefilter_texture);
  m_frontend->destroy_texture(RX_RENDER_TAG("ibl: scale bias"), m_scale_bias_texture);
}

void ImageBasedLighting::render(Frontend::TextureCM* _environment, Size _irradiance_map_size) {
  // NOTE(dweiler): Artifically limit the maximum size of IRM to avoid TDR.
  _irradiance_map_size = Algorithm::min(_irradiance_map_size, 32_z);

  Frontend::Technique* irradiance_technique = m_frontend->find_technique_by_name("irradiance_map");
  Frontend::Technique* prefilter_technique = m_frontend->find_technique_by_name("prefilter_environment_map");

  // Destroy the old textures irradiance and prefilter textures and create new ones.
  m_frontend->destroy_texture(RX_RENDER_TAG("ibl: irradiance"), m_irradiance_texture);
  m_frontend->destroy_texture(RX_RENDER_TAG("ibl: prefilter"), m_prefilter_texture);

  m_irradiance_texture = m_frontend->create_textureCM(RX_RENDER_TAG("ibl: irradiance"));
  m_irradiance_texture->record_format(Frontend::Texture::DataFormat::k_rgba_u8);
  m_irradiance_texture->record_type(Frontend::Texture::Type::ATTACHMENT);
  m_irradiance_texture->record_levels(1);
  m_irradiance_texture->record_dimensions({_irradiance_map_size, _irradiance_map_size});
  m_irradiance_texture->record_filter({true, false, false});
  m_irradiance_texture->record_wrap({
                                            Frontend::Texture::WrapType::k_clamp_to_edge,
                                            Frontend::Texture::WrapType::k_clamp_to_edge,
                                            Frontend::Texture::WrapType::k_clamp_to_edge});
  m_frontend->initialize_texture(RX_RENDER_TAG("ibl: irradiance"), m_irradiance_texture);

  static constexpr const Size k_max_prefilter_levels{5};
  m_prefilter_texture = m_frontend->create_textureCM(RX_RENDER_TAG("ibl: prefilter"));
  m_prefilter_texture->record_format(Frontend::Texture::DataFormat::k_rgba_u8);
  m_prefilter_texture->record_type(Frontend::Texture::Type::ATTACHMENT);
  m_prefilter_texture->record_levels(k_max_prefilter_levels + 1);
  m_prefilter_texture->record_dimensions(_environment->dimensions());
  m_prefilter_texture->record_filter({true, false, true});
  m_prefilter_texture->record_wrap({
                                           Frontend::Texture::WrapType::k_clamp_to_edge,
                                           Frontend::Texture::WrapType::k_clamp_to_edge,
                                           Frontend::Texture::WrapType::k_clamp_to_edge});
  m_frontend->initialize_texture(RX_RENDER_TAG("irradiance map"), m_prefilter_texture);

  // Render irradiance map.
  {
    Frontend::Target* target{m_frontend->create_target(RX_RENDER_TAG("ibl: irradiance"))};
    target->attach_texture(m_irradiance_texture, 0);
    m_frontend->initialize_target(RX_RENDER_TAG("ibl: irradiance"), target);

    Frontend::Program* program{*irradiance_technique};
    program->uniforms()[1].record_int(static_cast<int>(_irradiance_map_size));

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

    Frontend::Textures draw_textures;
    draw_textures.add(_environment);

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
    for (Size i{0}; i <= k_max_prefilter_levels; i++) {
      Frontend::Target* target{m_frontend->create_target(RX_RENDER_TAG("ibl: prefilter"))};
      target->attach_texture(m_prefilter_texture, i);
      m_frontend->initialize_target(RX_RENDER_TAG("ibl: prefilter"), target);

      const Float32 roughness{static_cast<Float32>(i) / k_max_prefilter_levels};
      Frontend::Program* program{*prefilter_technique};
      program->uniforms()[1].record_float(roughness);

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

      Frontend::Textures draw_textures;
      draw_textures.add(_environment);

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
