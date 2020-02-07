#ifndef RX_RENDER_BACKEND_VK_RENDERPASS_H
#define RX_RENDER_BACKEND_VK_RENDERPASS_H

#define VK_NO_PROTOTYPES
#include "vulkan/vulkan.h"

#include "helper.h"
#include "draw.h"

#include "rx/core/array.h"
#include "rx/core/vector.h"

#include "rx/render/frontend/target.h"

namespace rx::render::backend {

namespace detail_vk {
  
  struct context;
  
  struct target {
    
    void construct(context& ctx_, frontend::target* target);
    VkRenderPass make_renderpass(context& ctx_, frame_render::renderpass_info& rpinfo);
    VkFramebuffer make_framebuffer(context& ctx_, frontend::target* target, VkRenderPass renderpass);
    void destroy(context& ctx_, frontend::target* target);
    
  };
  
}

}

#endif
