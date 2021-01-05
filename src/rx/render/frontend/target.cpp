#include "rx/render/frontend/target.h"
#include "rx/render/frontend/command.h"
#include "rx/render/frontend/context.h"

namespace Rx::Render::Frontend {

Target::Target(Context* _frontend)
  : Resource{_frontend, Resource::Type::TARGET}
  , m_depth_texture{nullptr}
  , m_stencil_texture{nullptr}
  , m_attachments{_frontend->allocator()}
  , m_flags{0}
{
}

void Target::destroy() {
  if (owns_depth() && owns_stencil()) {
    m_frontend->destroy_texture_unlocked(RX_RENDER_TAG("target depth stencil"),
      m_depth_stencil_texture);
  } else if (owns_depth()) {
    m_frontend->destroy_texture_unlocked(RX_RENDER_TAG("target depth"),
      m_depth_texture);
  } else if (owns_stencil()) {
    m_frontend->destroy_texture_unlocked(RX_RENDER_TAG("target stencil"),
      m_stencil_texture);
  }
}

void Target::request_depth(Texture::DataFormat _format, const Math::Vec2z& _dimensions) {
  RX_ASSERT(!is_swapchain(), "request on swapchain");
  RX_ASSERT(!(m_flags & HAS_DEPTH), "already has depth attachment");
  RX_ASSERT(!(m_flags & HAS_STENCIL), "use combined depth stencil");
  RX_ASSERT(Texture::is_depth_format(_format), "not a valid depth format");

  if (m_flags & DIMENSIONS) {
    RX_ASSERT(_dimensions == m_dimensions, "invalid dimensions");
  } else {
    m_dimensions = _dimensions;
    m_flags |= DIMENSIONS;
  }

  m_depth_texture = m_frontend->create_texture2D(RX_RENDER_TAG("target depth"));
  m_depth_texture->record_format(_format);
  m_depth_texture->record_type(Texture::Type::ATTACHMENT);
  m_depth_texture->record_filter({false, false, false});
  m_depth_texture->record_dimensions(_dimensions);
  m_depth_texture->record_wrap({Texture::WrapType::CLAMP_TO_EDGE,
                                Texture::WrapType::CLAMP_TO_EDGE});
  m_frontend->initialize_texture(RX_RENDER_TAG("target depth"), m_depth_texture);

  m_flags |= OWNS_DEPTH;
  m_flags |= HAS_DEPTH;

  update_resource_usage();
}

void Target::request_stencil(Texture::DataFormat _format, const Math::Vec2z& _dimensions) {
  RX_ASSERT(!is_swapchain(), "request on swapchain");
  RX_ASSERT(!(m_flags & HAS_STENCIL), "already has stencil attachment");
  RX_ASSERT(!(m_flags & HAS_DEPTH), "use combined depth stencil");
  RX_ASSERT(Texture::is_stencil_format(_format), "not a valid stencil format");

  if (m_flags & DIMENSIONS) {
    RX_ASSERT(_dimensions == m_dimensions, "invalid dimensions");
  }

  m_stencil_texture = m_frontend->create_texture2D(RX_RENDER_TAG("target stencil"));
  m_stencil_texture->record_format(_format);
  m_stencil_texture->record_type(Texture::Type::ATTACHMENT);
  m_stencil_texture->record_filter({false, false, false});
  m_stencil_texture->record_dimensions(_dimensions);
  m_stencil_texture->record_wrap({Texture::WrapType::CLAMP_TO_EDGE,
                                  Texture::WrapType::CLAMP_TO_EDGE});
  m_frontend->initialize_texture(RX_RENDER_TAG("target stencil"), m_stencil_texture);

  m_flags |= OWNS_STENCIL;
  m_flags |= HAS_STENCIL;

  update_resource_usage();
}

void Target::request_depth_stencil(Texture::DataFormat _format, const Math::Vec2z& _dimensions) {
  RX_ASSERT(!is_swapchain(), "request on swapchain");
  RX_ASSERT(!(m_flags & HAS_DEPTH), "already has depth attachment");
  RX_ASSERT(!(m_flags & HAS_STENCIL), "already had stencil attachment");
  RX_ASSERT(Texture::is_depth_stencil_format(_format), "not a valid depth stencil format");

  if (m_flags & DIMENSIONS) {
    RX_ASSERT(_dimensions == m_dimensions, "invalid dimensions");
  } else {
    m_dimensions = _dimensions;
    m_flags |= DIMENSIONS;
  }

  m_depth_stencil_texture = m_frontend->create_texture2D(RX_RENDER_TAG("target depth stencil"));
  m_depth_stencil_texture->record_format(_format);
  m_depth_stencil_texture->record_type(Texture::Type::ATTACHMENT);
  m_depth_stencil_texture->record_filter({false, false, false});
  m_depth_stencil_texture->record_levels(1);
  m_depth_stencil_texture->record_dimensions(_dimensions);
  m_depth_stencil_texture->record_wrap({Texture::WrapType::CLAMP_TO_EDGE,
                                        Texture::WrapType::CLAMP_TO_EDGE});
  m_frontend->initialize_texture(RX_RENDER_TAG("target depth stencil"), m_depth_stencil_texture);

  m_flags |= OWNS_DEPTH;
  m_flags |= OWNS_STENCIL;

  m_flags |= HAS_DEPTH;
  m_flags |= HAS_STENCIL;

  update_resource_usage();
}

void Target::attach_depth(Texture2D* _depth) {
  RX_ASSERT(!is_swapchain(), "cannot attach to swapchain");
  RX_ASSERT(!(m_flags & HAS_DEPTH), "depth already attached");
  RX_ASSERT(!(m_flags & HAS_STENCIL), "use combined depth stencil");
  RX_ASSERT(_depth->is_depth_format(), "not a depth format texture");
  RX_ASSERT(_depth->type() == Texture::Type::ATTACHMENT, "not attachable texture");

  if (m_flags & DIMENSIONS) {
    RX_ASSERT(_depth->dimensions() == m_dimensions, "invalid dimensions");
  } else {
    m_dimensions = _depth->dimensions();
    m_flags |= DIMENSIONS;
  }

  m_depth_texture = _depth;
  m_flags |= HAS_DEPTH;

  update_resource_usage();
}

void Target::attach_stencil(Texture2D* _stencil) {
  RX_ASSERT(!is_swapchain(), "cannot attach to swapchain");
  RX_ASSERT(!(m_flags & HAS_STENCIL), "stencil already attached");
  RX_ASSERT(!(m_flags & HAS_DEPTH), "use combined depth stencil");
  RX_ASSERT(_stencil->is_stencil_format(), "not a stencil format texture");
  RX_ASSERT(_stencil->type() == Texture::Type::ATTACHMENT, "not attachable texture");

  if (m_flags & DIMENSIONS) {
    RX_ASSERT(_stencil->dimensions() == m_dimensions, "invalid dimensions");
  } else {
    m_dimensions = _stencil->dimensions();
    m_flags |= DIMENSIONS;
  }

  m_stencil_texture = _stencil;
  m_flags |= HAS_STENCIL;

  update_resource_usage();
}

void Target::attach_depth_stencil(Texture2D* _depth_stencil) {
  RX_ASSERT(!is_swapchain(), "cannot attach to swapchain");
  RX_ASSERT(!(m_flags & HAS_DEPTH), "depth aleady attached");
  RX_ASSERT(!(m_flags & HAS_STENCIL), "stencil already attached");
  RX_ASSERT(_depth_stencil->is_depth_stencil_format(), "not a depth stencil format texture");
  RX_ASSERT(_depth_stencil->type() == Texture::Type::ATTACHMENT, "not attachable texture");

  if (m_flags & DIMENSIONS) {
    RX_ASSERT(_depth_stencil->dimensions() == m_dimensions, "invalid dimensions");
  } else {
    m_dimensions = _depth_stencil->dimensions();
    m_flags |= DIMENSIONS;
  }

  m_depth_stencil_texture = _depth_stencil;

  m_flags |= HAS_DEPTH;
  m_flags |= HAS_STENCIL;

  update_resource_usage();
}

void Target::attach_texture(Texture2D* _texture, Size _level) {
  RX_ASSERT(!is_swapchain(), "cannot attach to swapchain");
  RX_ASSERT(_texture->type() == Texture::Type::ATTACHMENT,
    "not attachable texture");
  RX_ASSERT(_texture->is_level_in_range(_level), "level out of bounds");

  const auto& dimensions{_texture->info_for_level(_level).dimensions};

  if (m_flags & DIMENSIONS) {
    RX_ASSERT(dimensions == m_dimensions, "invalid dimensions");
  }

  RX_ASSERT(!has_texture(_texture), "texture already attached");

  if (!(m_flags & DIMENSIONS)) {
    m_dimensions = dimensions;
    m_flags |= DIMENSIONS;
  }

  Attachment new_attachment;
  new_attachment.kind = Attachment::Type::TEXTURE2D;
  new_attachment.level = _level;
  new_attachment.as_texture2D.texture = _texture;
  m_attachments.push_back(new_attachment);

  update_resource_usage();
}

void Target::attach_texture(TextureCM* _texture, TextureCM::Face _face, Size _level) {
  RX_ASSERT(!is_swapchain(), "cannot attach to swapchain");
  RX_ASSERT(_texture->type() == Texture::Type::ATTACHMENT,
    "not attachable texture");
  RX_ASSERT(_texture->is_level_in_range(_level), "level out of bounds");

  const auto& dimensions = _texture->info_for_level(_level).dimensions;

  if (m_flags & DIMENSIONS) {
    RX_ASSERT(dimensions == m_dimensions, "invalid dimensions");
  }

  // Don't allow attaching the same cubemap face multiple times.
  RX_ASSERT(!has_cubemap_face(_texture, _face), "texture already attached");

  if (!(m_flags & DIMENSIONS)) {
    m_dimensions = dimensions;
    m_flags |= DIMENSIONS;
  }

  Attachment new_attachment;
  new_attachment.kind = Attachment::Type::TEXTURECM;
  new_attachment.level = _level;
  new_attachment.as_textureCM.texture = _texture;
  new_attachment.as_textureCM.face = _face;
  m_attachments.push_back(new_attachment);

  update_resource_usage();
}

void Target::validate() const {
  RX_ASSERT(m_flags & DIMENSIONS, "dimensions not recorded");

  if (m_flags & SWAPCHAIN) {
    // There is one attachment given to the swapchain target by |interface|.
    RX_ASSERT(m_attachments.size() == 1,
      "swapchain cannot have attachments");
  } else if (!m_depth_texture && !m_stencil_texture) {
    RX_ASSERT(!m_attachments.is_empty(), "no attachments");
  }
}

void Target::update_resource_usage() {
  const auto rt_usage{[](const auto* _texture) {
    return _texture->dimensions().area() * Texture::bits_per_pixel(_texture->format()) / 8;
  }};

  // Calculate memory usage for each attachment texture.
  Float32 usage{0};
  m_attachments.each_fwd([&](const Attachment& _attachment) {
    switch (_attachment.kind) {
    case Attachment::Type::TEXTURE2D:
      usage += rt_usage(_attachment.as_texture2D.texture);
      break;
    case Attachment::Type::TEXTURECM:
      usage += rt_usage(_attachment.as_textureCM.texture);
      break;
    }
  });

  // Calculate memory usage of any depth, stencil or depth stencil attachments.
  if (has_depth_stencil()) {
    usage += rt_usage(m_depth_stencil_texture);
  } else {
    if (has_depth()) {
      usage += rt_usage(m_depth_texture);
    } else if (has_stencil()) {
      usage += rt_usage(m_stencil_texture);
    }
  }

  Resource::update_resource_usage(static_cast<Size>(usage));
}

bool Target::has_texture(const Texture* _texture) const {
  if (_texture->is_color_format()) {
    auto find = [_texture](const Attachment& _attachment) {
      return _attachment.texture_is(_texture);
    };
    return m_attachments.find_if(find);
  } else if (_texture->is_depth_stencil_format()) {
    return has_depth_stencil() && m_depth_stencil_texture == _texture;
  } else if (_texture->is_depth_format()) {
    return has_depth() && m_depth_texture == _texture;
  } else if (_texture->is_stencil_format()) {
    return has_stencil() && m_stencil_texture == _texture;
  }
  return false;
}

bool Target::has_cubemap_face(const TextureCM* _texture, TextureCM::Face _face) const {
  const auto n_attachments = m_attachments.size();
  for (Size i = 0; i < n_attachments; i++) {
    const auto& attachment = m_attachments[i];
    if (attachment.texture_is(_texture) && attachment.as_textureCM.face == _face) {
      return true;
    }
  }
  return false;
}

bool Target::has_feedback(const Texture* _texture, const Buffers& _draw_buffers) const {
  if (_texture->is_color_format()) {
    const auto n_draw_buffers = _draw_buffers.size();
    for (Size i = 0; i < n_draw_buffers; i++) {
      const auto& attachment = m_attachments[_draw_buffers[i]];
      if (attachment.texture_is(_texture)) {
        return true;
      }
    }
  } else if (_texture->is_depth_stencil_format()) {
    return has_depth_stencil() && m_depth_stencil_texture == _texture;
  } else if (_texture->is_depth_format()) {
    return has_depth() && m_depth_texture == _texture;
  } else if (_texture->is_stencil_format()) {
    return has_stencil() && m_stencil_texture == _texture;
  }
  return false;
}


} // namespace Rx::Render::Frontend
