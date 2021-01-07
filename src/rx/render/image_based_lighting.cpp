
#include "rx/render/image_based_lighting.h"

#include "rx/render/frontend/technique.h"
#include "rx/render/frontend/program.h"
#include "rx/render/frontend/context.h"
#include "rx/render/frontend/texture.h"
#include "rx/render/frontend/target.h"
#include "rx/render/frontend/state.h"

namespace Rx::Render {

// [IrradianceMap]
Optional<IrradianceMap> IrradianceMap::create(Frontend::Context* _frontend, Size _resolution) {
  auto tag = "irradiance map";
  auto technique = _frontend->find_technique_by_name("irradiance_map");
  if (!technique) {
    return nullopt;
  }

  auto target = _frontend->create_target(RX_RENDER_TAG(tag));
  auto texture = _frontend->create_textureCM(RX_RENDER_TAG(tag));
  if (!target || !texture) {
    _frontend->destroy_texture(RX_RENDER_TAG(tag), texture);
    _frontend->destroy_target(RX_RENDER_TAG(tag), target);
    return nullopt;
  }

  texture->record_levels(1);
  texture->record_format(Frontend::Texture::DataFormat::RGBA_U8);
  texture->record_type(Frontend::Texture::Type::ATTACHMENT);
  texture->record_dimensions({_resolution, _resolution});
  texture->record_filter({true, false, false}); // LINEAR
  texture->record_wrap({Frontend::Texture::WrapType::REPEAT,
                        Frontend::Texture::WrapType::REPEAT,
                        Frontend::Texture::WrapType::REPEAT});
  _frontend->initialize_texture(RX_RENDER_TAG(tag), texture);

  target->attach_texture(texture, 0);
  _frontend->initialize_target(RX_RENDER_TAG(tag), target);

  IrradianceMap result;
  result.m_frontend = _frontend;
  result.m_target = target;
  result.m_texture = texture;
  result.m_program = *technique;
  result.m_resolution = _resolution;

  return result;
}

void IrradianceMap::release() {
  if (!m_frontend) {
    return;
  }
  m_frontend->destroy_target(RX_RENDER_TAG("irradiance map"), m_target);
  m_frontend->destroy_texture(RX_RENDER_TAG("irradiance map"), m_texture);
}

void IrradianceMap::render(Frontend::TextureCM* _env_map) {
  if (m_environment_map != _env_map) {
    m_current_face = 0;
    m_environment_map = _env_map;
  }
  render_next_face();
}

void IrradianceMap::render_next_face() {
  // Nothing to render.
  if (!m_frontend || m_current_face >= 6) {
    return;
  }

  Frontend::Textures textures;
  m_program->uniforms()[0].record_sampler(textures.add(m_environment_map));
  m_program->uniforms()[1].record_int(m_current_face);
  m_program->uniforms()[2].record_int(Sint32(m_resolution) * 4);

  Frontend::Buffers buffers;
  buffers.add(m_current_face);

  Frontend::State state;
  state.viewport.record_dimensions({m_resolution, m_resolution});
  state.cull.record_enable(false);

  m_frontend->draw(
    RX_RENDER_TAG("irradiance map"),
    state,
    m_target,
    buffers,
    nullptr,
    m_program,
    3,
    0,
    0,
    0,
    0,
    Frontend::PrimitiveType::TRIANGLES,
    textures);

  m_current_face++;
}

// [PrefilteredEnvironmentMap]
Optional<PrefilteredEnvironmentMap> PrefilteredEnvironmentMap::create(Frontend::Context* _frontend, Size _resolution) {
  auto tag = "prefiltered environment map";
  auto technique = _frontend->find_technique_by_name("prefilter_environment_map");
  if (!technique) {
    return nullopt;
  }

  auto target = _frontend->create_target(RX_RENDER_TAG(tag));
  auto texture = _frontend->create_textureCM(RX_RENDER_TAG(tag));
  if (!target || !texture) {
    _frontend->destroy_texture(RX_RENDER_TAG(tag), texture);
    _frontend->destroy_target(RX_RENDER_TAG(tag), target);
    return nullopt;
  }

  texture->record_levels(5 + 1);
  texture->record_format(Frontend::Texture::DataFormat::RGBA_F16);
  texture->record_type(Frontend::Texture::Type::ATTACHMENT);
  texture->record_dimensions({_resolution, _resolution});
  texture->record_filter({true, false, false}); // LINEAR
  texture->record_wrap({Frontend::Texture::WrapType::REPEAT,
                        Frontend::Texture::WrapType::REPEAT,
                        Frontend::Texture::WrapType::REPEAT});
  _frontend->initialize_texture(RX_RENDER_TAG(tag), texture);

  target->attach_texture(texture, 0);
  _frontend->initialize_target(RX_RENDER_TAG(tag), target);

  PrefilteredEnvironmentMap result;
  result.m_frontend = _frontend;
  result.m_target = target;
  result.m_texture = texture;
  result.m_program = *technique;
  result.m_resolution = _resolution;

  return result;
}

void PrefilteredEnvironmentMap::release() {
  if (!m_frontend) {
    return;
  }
  m_frontend->destroy_target(RX_RENDER_TAG("prefiltered encironment map"), m_target);
  m_frontend->destroy_texture(RX_RENDER_TAG("prefiltered environment map"), m_texture);
}

void PrefilteredEnvironmentMap::render(Frontend::TextureCM* _env_map) {
  if (m_environment_map != _env_map) {
    m_current_face = 0;
    m_environment_map = _env_map;
  }
  render_next_face();
}

void PrefilteredEnvironmentMap::render_next_face() {
  // Nothing to render.
  if (!m_frontend || m_current_face >= 6) {
    return;
  }

  Frontend::Textures textures;
  m_program->uniforms()[0].record_sampler(textures.add(m_environment_map));
  m_program->uniforms()[1].record_int(m_current_face);

  Frontend::Buffers buffers;
  buffers.add(m_current_face);

  Frontend::State state;
  state.cull.record_enable(false);

  for (Size i = 0; i <= 5; i++) {
    auto mipmap_size = m_resolution >> i;
    auto roughness = Float32(i) / 5;
    state.viewport.record_dimensions({mipmap_size, mipmap_size});
    m_program->uniforms()[2].record_float(roughness);

    m_frontend->draw(
      RX_RENDER_TAG("prefiltered environment map"),
      state,
      m_target,
      buffers,
      nullptr,
      m_program,
      3,
      0,
      0,
      0,
      0,
      Frontend::PrimitiveType::TRIANGLES,
      textures);
  }

  m_current_face++;
}

Optional<ImageBasedLighting> ImageBasedLighting::create(
  Frontend::Context* _frontend, Size _irradiance_size, Size _prefilter_size)
{
  auto scale_bias_technique = _frontend->find_technique_by_name("brdf_integration");
  if (!scale_bias_technique) {
    return nullopt;
  }

  auto scale_bias_texture = _frontend->create_texture2D(RX_RENDER_TAG("scale bias"));
  auto scale_bias_target = _frontend->create_target(RX_RENDER_TAG("scale bias"));
  if (!scale_bias_texture || !scale_bias_target) {
    _frontend->destroy_target(RX_RENDER_TAG("scale bias"), scale_bias_target);
    _frontend->destroy_texture(RX_RENDER_TAG("scale bias"), scale_bias_texture);
    return nullopt;
  }

  scale_bias_texture->record_type(Frontend::Texture::Type::ATTACHMENT);
  scale_bias_texture->record_levels(1);
  scale_bias_texture->record_format(Frontend::Texture::DataFormat::RGBA_U8);
  scale_bias_texture->record_filter({true, false, false}); // LINEAR
  scale_bias_texture->record_dimensions({256, 256});
  scale_bias_texture->record_wrap({Frontend::Texture::WrapType::CLAMP_TO_EDGE,
                                   Frontend::Texture::WrapType::CLAMP_TO_EDGE});
  _frontend->initialize_texture(RX_RENDER_TAG("scale bias"), scale_bias_texture);

  scale_bias_target->attach_texture(scale_bias_texture, 0);
  _frontend->initialize_target(RX_RENDER_TAG("scale bias"), scale_bias_target);

  // Render the LUT.
  Frontend::Buffers buffers;
  buffers.add(0);

  Frontend::State state;
  state.viewport.record_dimensions({256, 256});
  state.cull.record_enable(false);

  _frontend->draw(
    RX_RENDER_TAG("scale bias"),
    state,
    scale_bias_target,
    buffers,
    nullptr,
    *scale_bias_technique,
    3,
    0,
    0,
    0,
    0,
    Frontend::PrimitiveType::TRIANGLES,
    {});

  _frontend->destroy_target(RX_RENDER_TAG("scale bias"), scale_bias_target);

  auto irradiance_map = IrradianceMap::create(_frontend, _irradiance_size);
  auto prefiltered_environment_map = PrefilteredEnvironmentMap::create(_frontend, _prefilter_size);
  if (!irradiance_map || !prefiltered_environment_map) {
    _frontend->destroy_texture(RX_RENDER_TAG("scale bias"), scale_bias_texture);
    return nullopt;
  }

  ImageBasedLighting result;
  result.m_frontend = _frontend;
  result.m_scale_bias_texture = scale_bias_texture;
  result.m_irradiance_map = Utility::move(*irradiance_map);
  result.m_prefiltered_environment_map = Utility::move(*prefiltered_environment_map);

  return result;
}

void ImageBasedLighting::release() {
  if (!m_frontend) {
    return;
  }
  m_frontend->destroy_texture(RX_RENDER_TAG("scale bias"), m_scale_bias_texture);
}

void ImageBasedLighting::render(Frontend::TextureCM* _env_map) {
  m_irradiance_map.render(_env_map);
  m_prefiltered_environment_map.render(_env_map);
}

} // namespace Rx::Render
