#include "rx/render/frontend/target.h"
#include "rx/render/frontend/command.h"
#include "rx/render/frontend/interface.h"

#include "rx/console/interface.h"
#include "rx/console/variable.h"

namespace rx::render::frontend {

target::target(interface* _frontend)
  : resource{_frontend, resource::type::k_target}
  , m_depth_texture{nullptr}
  , m_stencil_texture{nullptr}
  , m_attachments{_frontend->allocator()}
  , m_flags{0}
{
}

target::~target() {
  const bool owns_depth{!!(m_flags & k_owns_depth)};
  const bool owns_stencil{!!(m_flags & k_owns_stencil)};
  if (owns_depth && owns_stencil) {
    m_frontend->destroy_texture_unlocked(RX_RENDER_TAG("target depth stencil"),
      m_depth_stencil_texture);
  } else if (owns_depth) {
    m_frontend->destroy_texture_unlocked(RX_RENDER_TAG("target depth"),
      m_depth_texture);
  } else if (owns_stencil) {
    m_frontend->destroy_texture_unlocked(RX_RENDER_TAG("target stencil"),
      m_stencil_texture);
  }
}

void target::request_depth(texture::data_format _format, const math::vec2z& _dimensions) {
  RX_ASSERT(!is_swapchain(), "request on swapchain");
  RX_ASSERT(!(m_flags & k_has_depth), "already has depth attachment");
  RX_ASSERT(!(m_flags & k_has_stencil), "use combined depth stencil");
  RX_ASSERT(texture::is_depth_format(_format), "not a valid depth format");

  if (m_flags & k_dimensions) {
    RX_ASSERT(_dimensions == m_dimensions, "invalid dimensions");
  } else {
    m_dimensions = _dimensions;
    m_flags |= k_dimensions;
  }

  m_depth_texture = m_frontend->create_texture2D(RX_RENDER_TAG("target depth"));
  m_depth_texture->record_format(_format);
  m_depth_texture->record_type(texture::type::k_attachment);
  m_depth_texture->record_filter({false, false, false});
  m_depth_texture->record_dimensions(_dimensions);
  m_depth_texture->record_wrap({
    texture::wrap_type::k_clamp_to_edge,
    texture::wrap_type::k_clamp_to_edge});
  m_frontend->initialize_texture(RX_RENDER_TAG("target depth"), m_depth_texture);

  m_flags |= k_owns_depth;
  m_flags |= k_has_depth;

  update_resource_usage();
}

void target::request_stencil(texture::data_format _format, const math::vec2z& _dimensions) {
  RX_ASSERT(!is_swapchain(), "request on swapchain");
  RX_ASSERT(!(m_flags & k_has_stencil), "already has stencil attachment");
  RX_ASSERT(!(m_flags & k_has_depth), "use combined depth stencil");
  RX_ASSERT(texture::is_stencil_format(_format), "not a valid stencil format");

  if (m_flags & k_dimensions) {
    RX_ASSERT(_dimensions == m_dimensions, "invalid dimensions");
  }

  m_stencil_texture = m_frontend->create_texture2D(RX_RENDER_TAG("target stencil"));
  m_stencil_texture->record_format(_format);
  m_stencil_texture->record_type(texture::type::k_attachment);
  m_stencil_texture->record_filter({false, false, false});
  m_stencil_texture->record_dimensions(_dimensions);
  m_stencil_texture->record_wrap({
    texture::wrap_type::k_clamp_to_edge,
    texture::wrap_type::k_clamp_to_edge});
  m_frontend->initialize_texture(RX_RENDER_TAG("target stencil"), m_stencil_texture);

  m_flags |= k_owns_stencil;
  m_flags |= k_has_stencil;

  update_resource_usage();
}

void target::request_depth_stencil(texture::data_format _format, const math::vec2z& _dimensions) {
  RX_ASSERT(!is_swapchain(), "request on swapchain");
  RX_ASSERT(!(m_flags & k_has_depth), "already has depth attachment");
  RX_ASSERT(!(m_flags & k_has_stencil), "already had stencil attachment");
  RX_ASSERT(texture::is_depth_stencil_format(_format), "not a valid depth stencil format");

  if (m_flags & k_dimensions) {
    RX_ASSERT(_dimensions == m_dimensions, "invalid dimensions");
  } else {
    m_dimensions = _dimensions;
    m_flags |= k_dimensions;
  }

  m_depth_stencil_texture = m_frontend->create_texture2D(RX_RENDER_TAG("target depth stencil"));
  m_depth_stencil_texture->record_format(_format);
  m_depth_stencil_texture->record_type(texture::type::k_attachment);
  m_depth_stencil_texture->record_filter({false, false, false});
  m_depth_stencil_texture->record_levels(1);
  m_depth_stencil_texture->record_dimensions(_dimensions);
  m_depth_stencil_texture->record_wrap({
    texture::wrap_type::k_clamp_to_edge,
    texture::wrap_type::k_clamp_to_edge});
  m_frontend->initialize_texture(RX_RENDER_TAG("target depth stencil"), m_depth_stencil_texture);

  m_flags |= k_owns_depth;
  m_flags |= k_owns_stencil;

  m_flags |= k_has_depth;
  m_flags |= k_has_stencil;

  update_resource_usage();
}

void target::attach_depth(texture2D* _depth) {
  RX_ASSERT(!is_swapchain(), "cannot attach to swapchain");
  RX_ASSERT(!(m_flags & k_has_depth), "depth already attached");
  RX_ASSERT(!(m_flags & k_has_stencil), "use combined depth stencil");
  RX_ASSERT(_depth->is_depth_format(), "not a depth format texture");
  RX_ASSERT(_depth->kind() == texture::type::k_attachment, "not attachable texture");

  if (m_flags & k_dimensions) {
    RX_ASSERT(_depth->dimensions() == m_dimensions, "invalid dimensions");
  } else {
    m_dimensions = _depth->dimensions();
    m_flags |= k_dimensions;
  }

  m_depth_texture = _depth;
  m_flags |= k_has_depth;

  update_resource_usage();
}

void target::attach_stencil(texture2D* _stencil) {
  RX_ASSERT(!is_swapchain(), "cannot attach to swapchain");
  RX_ASSERT(!(m_flags & k_has_stencil), "stencil already attached");
  RX_ASSERT(!(m_flags & k_has_depth), "use combined depth stencil");
  RX_ASSERT(_stencil->is_stencil_format(), "not a stencil format texture");
  RX_ASSERT(_stencil->kind() == texture::type::k_attachment, "not attachable texture");

  if (m_flags & k_dimensions) {
    RX_ASSERT(_stencil->dimensions() == m_dimensions, "invalid dimensions");
  } else {
    m_dimensions = _stencil->dimensions();
    m_flags |= k_dimensions;
  }

  m_stencil_texture = _stencil;
  m_flags |= k_has_stencil;

  update_resource_usage();
}

void target::attach_texture(texture2D* _texture, rx_size _level) {
  RX_ASSERT(!is_swapchain(), "cannot attach to swapchain");
  RX_ASSERT(_texture->kind() == texture::type::k_attachment,
    "not attachable texture");
  RX_ASSERT(_texture->is_level_in_range(_level), "level out of bounds");

  if (m_flags & k_dimensions) {
    RX_ASSERT(_texture->dimensions() == m_dimensions, "invalid dimensions");
  }

  m_attachments.each_fwd([_texture](const attachment& _attachment) {
    if (_attachment.kind == attachment::type::k_texture2D) {
      RX_ASSERT(_texture != _attachment.as_texture2D.texture,
        "texture already attached");
    }
  });

  if (!(m_flags & k_dimensions)) {
    m_dimensions = _texture->dimensions();
    m_flags |= k_dimensions;
  }

  attachment new_attachment;
  new_attachment.kind = attachment::type::k_texture2D;
  new_attachment.level = _level;
  new_attachment.as_texture2D.texture = _texture;
  m_attachments.push_back(new_attachment);

  update_resource_usage();
}

void target::attach_texture(textureCM* _texture, textureCM::face _face, rx_size _level) {
  RX_ASSERT(!is_swapchain(), "cannot attach to swapchain");
  RX_ASSERT(_texture->kind() == texture::type::k_attachment,
    "not attachable texture");
  RX_ASSERT(_texture->is_level_in_range(_level), "level out of bounds");

  if (m_flags & k_dimensions) {
    RX_ASSERT(_texture->dimensions() == m_dimensions, "invalid dimensions");
  }

  // Don't allow attaching the same cubemap face multiple times.
  m_attachments.each_fwd([_texture, _face](const attachment& _attachment) {
    if (_attachment.kind == attachment::type::k_textureCM
      && _attachment.as_textureCM.texture == _texture)
    {
      RX_ASSERT(_attachment.as_textureCM.face != _face,
        "texture already attached");
    }
  });

  if (!(m_flags & k_dimensions)) {
    m_dimensions = _texture->dimensions();
    m_flags |= k_dimensions;
  }

  attachment new_attachment;
  new_attachment.kind = attachment::type::k_textureCM;
  new_attachment.level = _level;
  new_attachment.as_textureCM.texture = _texture;
  new_attachment.as_textureCM.face = _face;
  m_attachments.push_back(new_attachment);

  update_resource_usage();
}

void target::validate() const {
  RX_ASSERT(m_flags & k_dimensions, "dimensions not recorded");

  if (m_flags & k_swapchain) {
    // There is one attachment given to the swapchain target by |interface|.
    RX_ASSERT(m_attachments.size() == 1,
      "swapchain cannot have attachments");
  } else if (!m_depth_texture && !m_stencil_texture) {
    RX_ASSERT(!m_attachments.is_empty(), "no attachments");
  }
}

void target::update_resource_usage() {
  const auto rt_usage{[](const auto* _texture) {
    return _texture->dimensions().area() * texture::byte_size_of_format(_texture->format());
  }};

  // Calculate memory usage for each attachment texture.
  rx_f32 usage{0};
  m_attachments.each_fwd([&](const attachment& _attachment) {
    switch (_attachment.kind) {
    case attachment::type::k_texture2D:
      usage += rt_usage(_attachment.as_texture2D.texture);
      break;
    case attachment::type::k_textureCM:
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

  resource::update_resource_usage(static_cast<rx_size>(usage));
}

} // namespace rx::render::frontend
