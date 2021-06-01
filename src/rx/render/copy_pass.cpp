#include "rx/render/copy_pass.h"

#include "rx/render/frontend/context.h"
#include "rx/render/frontend/texture.h"
#include "rx/render/frontend/target.h"
#include "rx/render/frontend/technique.h"
#include "rx/render/frontend/program.h"
#include "rx/render/frontend/state.h"

namespace Rx::Render {

Optional<CopyPass> CopyPass::create(Frontend::Context* _frontend,
  const Options& _options)
{
  auto technique = _frontend->find_technique_by_name("copy");
  if (!technique) {
    return nullopt;
  }

  auto texture = _frontend->create_texture2D(RX_RENDER_TAG("CopyPass"));
  if (!texture) {
    return nullopt;
  }

  texture->record_type(Frontend::Texture::Type::ATTACHMENT);
  texture->record_format(_options.format);
  texture->record_filter({true, false, false});
  texture->record_levels(1);
  texture->record_dimensions(_options.dimensions);
  texture->record_wrap({
    Frontend::Texture::WrapType::CLAMP_TO_EDGE,
    Frontend::Texture::WrapType::CLAMP_TO_EDGE});
  _frontend->initialize_texture(RX_RENDER_TAG("CopyPass"), texture);

  auto target = _frontend->create_target(RX_RENDER_TAG("CopyPass"));
  if (!target) {
    _frontend->destroy_texture(RX_RENDER_TAG("CopyPass"), texture);
    return nullopt;
  }

  target->attach_texture(texture, 0);
  _frontend->initialize_target(RX_RENDER_TAG("CopyPass"), target);

  return CopyPass {
    _frontend,
    target,
    texture,
    technique
  };
}

void CopyPass::release() {
  if (m_frontend) {
    m_frontend->destroy_target(RX_RENDER_TAG("CopyPass"), m_target);
    m_frontend->destroy_texture(RX_RENDER_TAG("CopyPass"), m_texture);
  }
}

void CopyPass::render(Frontend::Texture2D* _source) {
  const auto& dimensions = m_texture->dimensions();

  Frontend::Program* program = m_technique->configuration(0).basic();

  Frontend::Buffers draw_buffers;
  draw_buffers.add(0);

  Frontend::Textures draw_textures;
  program->uniforms()[0].record_sampler(draw_textures.add(_source));

  Frontend::State state;
  state.viewport.record_dimensions(dimensions);
  state.cull.record_enable(false);

  state.depth.record_test(false);
  state.depth.record_write(false);

  m_frontend->draw(
    RX_RENDER_TAG("CopyPass"),
    state,
    m_target,
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
}

bool CopyPass::recreate(const Options& _options) {
  if (auto result = create(m_frontend, _options)) {
    *this = Utility::move(*result);
    return true;
  }
  return false;
}

} // namespace Rx::Render
