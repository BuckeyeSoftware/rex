#include "rx/render/target.h"
#include "rx/render/command.h"
#include "rx/render/frontend.h"

namespace rx::render {

// checks if |_format| is a valid depth-only format
static bool is_valid_depth_format(texture::data_format _format) {
  switch (_format) {
  case texture::data_format::k_rgba_u8:
    return false;
  case texture::data_format::k_bgra_u8:
    return false;
  case texture::data_format::k_rgba_f16:
    return false;
  case texture::data_format::k_bgra_f16:
    return false;
  case texture::data_format::k_d16:
    return true;
  case texture::data_format::k_d24:
    return true;
  case texture::data_format::k_d32:
    return true;
  case texture::data_format::k_d32f:
    return true;
  case texture::data_format::k_d24_s8:
    return false;
  case texture::data_format::k_d32f_s8:
    return false;
  case texture::data_format::k_s8:
    return false;
  case texture::data_format::k_r_u8:
    return false;
  }
  return false;
}

// checks if |_format| is a valid stencil-only format
static bool is_valid_stencil_format(texture::data_format _format) {
  switch (_format) {
  case texture::data_format::k_rgba_u8:
    return false;
  case texture::data_format::k_bgra_u8:
    return false;
  case texture::data_format::k_rgba_f16:
    return false;
  case texture::data_format::k_bgra_f16:
    return false;
  case texture::data_format::k_d16:
    return false;
  case texture::data_format::k_d24:
    return false;
  case texture::data_format::k_d32:
    return false;
  case texture::data_format::k_d32f:
    return false;
  case texture::data_format::k_d24_s8:
    return false;
  case texture::data_format::k_d32f_s8:
    return false;
  case texture::data_format::k_s8:
    return true;
  case texture::data_format::k_r_u8:
    return false;
  }
  return false;
}

// checks if |_format| is a valid depth-stencil-only format
static bool is_valid_depth_stencil_format(texture::data_format _format) {
  switch (_format) {
  case texture::data_format::k_rgba_u8:
    return false;
  case texture::data_format::k_bgra_u8:
    return false;
  case texture::data_format::k_rgba_f16:
    return false;
  case texture::data_format::k_bgra_f16:
    return false;
  case texture::data_format::k_d16:
    return false;
  case texture::data_format::k_d24:
    return false;
  case texture::data_format::k_d32:
    return false;
  case texture::data_format::k_d32f:
    return false;
  case texture::data_format::k_d24_s8:
    return true;
  case texture::data_format::k_d32f_s8:
    return true;
  case texture::data_format::k_s8:
    return false;
  case texture::data_format::k_r_u8:
    return false;
  }
  return false;
}

target::target(frontend* _frontend)
  : resource{_frontend, resource::type::k_target}
  , m_depth_texture{nullptr}
  , m_stencil_texture{nullptr}
  , m_attachments{_frontend->allocator()}
  , m_owns{0}
  , m_is_swapchain{false}
{
}

target::~target() {
  if (m_owns != 0) {
    if (m_depth_texture == m_stencil_texture) {
      m_frontend->destroy_texture_unlocked(
        RX_RENDER_TAG("target depth stencil"),
        m_depth_stencil_texture);
    } else {
      if (m_owns & k_depth) {
        m_frontend->destroy_texture_unlocked(
          RX_RENDER_TAG("target depth"),
          m_depth_texture);
      }
      if (m_owns & k_stencil) {
        m_frontend->destroy_texture_unlocked(
          RX_RENDER_TAG("target stencil"),
          m_stencil_texture);
      }
    }
  }
}

void target::request_swapchain() {
  RX_ASSERT(m_owns == 0 && m_attachments.is_empty(), "target is not empty");
  m_is_swapchain = true;
}

void target::request_depth(texture::data_format _format, const math::vec2z& _dimensions) {
  RX_ASSERT(!is_swapchain(), "request on swapchain");
  RX_ASSERT(!m_depth_texture, "already has depth attachment");
  RX_ASSERT(!m_stencil_texture, "use combined depth stencil");
  RX_ASSERT(is_valid_depth_format(_format), "not a valid depth format");

  m_depth_texture = m_frontend->create_texture2D(RX_RENDER_TAG("target depth"));
  m_depth_texture->record_format(_format);
  m_depth_texture->record_type(texture::type::k_attachment);
  m_depth_texture->record_filter({ false, false, false });
  m_depth_texture->record_dimensions(_dimensions);
  m_depth_texture->record_wrap({
    texture::wrap_type::k_clamp_to_edge,
    texture::wrap_type::k_clamp_to_edge});
  m_frontend->initialize_texture(RX_RENDER_TAG("target depth"), m_depth_texture);

  m_owns |= k_depth;

  update_resource_usage();
}

void target::request_stencil(texture::data_format _format, const math::vec2z& _dimensions) {
  RX_ASSERT(!is_swapchain(), "request on swapchain");
  RX_ASSERT(!m_stencil_texture, "already has stencil attachment");
  RX_ASSERT(!m_depth_texture, "use combined depth stencil");
  RX_ASSERT(is_valid_stencil_format(_format), "not a valid stencil format");

  m_stencil_texture = m_frontend->create_texture2D(RX_RENDER_TAG("target stencil"));
  m_stencil_texture->record_format(_format);
  m_stencil_texture->record_type(texture::type::k_attachment);
  m_stencil_texture->record_filter({ false, false, false });
  m_stencil_texture->record_dimensions(_dimensions);
  m_stencil_texture->record_wrap({
    texture::wrap_type::k_clamp_to_edge,
    texture::wrap_type::k_clamp_to_edge});
  m_frontend->initialize_texture(RX_RENDER_TAG("target stencil"), m_stencil_texture);

  m_owns |= k_stencil;

  update_resource_usage();
}

void target::request_depth_stencil(texture::data_format _format, const math::vec2z& _dimensions) {
  RX_ASSERT(!is_swapchain(), "request on swapchain");
  RX_ASSERT(!m_depth_texture, "already has depth attachment");
  RX_ASSERT(!m_stencil_texture, "already had stencil attachment");
  RX_ASSERT(is_valid_depth_stencil_format(_format), "not a valid depth stencil format");

  m_depth_stencil_texture = m_frontend->create_texture2D(RX_RENDER_TAG("target depth stencil"));
  m_depth_stencil_texture->record_format(_format);
  m_depth_stencil_texture->record_type(texture::type::k_attachment);
  m_depth_stencil_texture->record_filter({ false, false, false });
  m_depth_stencil_texture->record_dimensions(_dimensions);
  m_depth_stencil_texture->record_wrap({
    texture::wrap_type::k_clamp_to_edge,
    texture::wrap_type::k_clamp_to_edge});
  m_frontend->initialize_texture(RX_RENDER_TAG("target depth stencil"), m_depth_stencil_texture);

  m_owns |= k_depth;
  m_owns |= k_stencil;

  update_resource_usage();
}

void target::attach_depth(texture2D* _depth) {
  RX_ASSERT(!is_swapchain(), "cannot attach to swapchain");
  RX_ASSERT(!m_depth_texture, "depth already attached");
  RX_ASSERT(is_valid_depth_format(_depth->format()), "not a depth format texture");
  RX_ASSERT(_depth->kind() == texture::type::k_attachment, "not attachable texture");

  m_depth_texture = _depth;

  update_resource_usage();
}

void target::attach_stencil(texture2D* _stencil) {
  RX_ASSERT(!is_swapchain(), "cannot attach to swapchain");
  RX_ASSERT(!m_stencil_texture, "stencil already attached");
  RX_ASSERT(is_valid_stencil_format(_stencil->format()), "not a stencil format texture");
  RX_ASSERT(_stencil->kind() == texture::type::k_attachment, "not attachable texture");

  m_stencil_texture = _stencil;

  update_resource_usage();
}

void target::attach_texture(texture2D* _texture) {
  RX_ASSERT(!is_swapchain(), "cannot attach to swapchain");
  RX_ASSERT(_texture->kind() == texture::type::k_attachment, "not attachable texture");

  m_attachments.each_fwd([_texture](texture2D* _attachment) {
    RX_ASSERT(_texture != _attachment, "texture already attached");
  });
  m_attachments.push_back(_texture);

  update_resource_usage();
}

void target::validate() const {
  if (m_is_swapchain) {
    RX_ASSERT(m_attachments.is_empty(), "swapchain cannot have attachments");
  } else {
    if (!m_depth_texture && !m_stencil_texture) {
      RX_ASSERT(!m_attachments.is_empty(), "no attachments");
    }
  }
}

void target::update_resource_usage() {
  const auto rt_usage{[](const texture2D* _texture){
    return _texture->dimensions().area() * texture::byte_size_of_format(_texture->format());
  }};

  // calcualte memory usage for each attachment texture
  rx_size usage{0};
  m_attachments.each_fwd([&](const texture2D* _texture) {
    usage += rt_usage(_texture);
  });

  // as well as any usage for depth and stencil textures
  if (m_depth_stencil_texture) {
    usage += rt_usage(m_depth_stencil_texture);
  } else {
    if (m_depth_texture) {
      usage += rt_usage(m_depth_texture);
    } else if (m_stencil_texture) {
      usage += rt_usage(m_stencil_texture);
    }
  }

  resource::update_resource_usage(usage);
}

} // namespace rx::render