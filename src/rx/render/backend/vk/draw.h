#ifndef RX_RENDER_BACKEND_VK_DRAW_H
#define RX_RENDER_BACKEND_VK_DRAW_H

#define VK_NO_PROTOTYPES
#include "vulkan/vulkan.h"

#include "helper.h"
#include "data_builder.h"

#include "rx/core/array.h"
#include "rx/core/vector.h"

#include "rx/render/frontend/buffer.h"
#include "rx/render/frontend/target.h"
#include "rx/render/frontend/program.h"
#include "rx/render/frontend/resource.h"
#include "rx/render/frontend/texture.h"
#include "rx/render/frontend/command.h"

namespace rx::render::backend {

namespace detail_vk {
  
  struct context;
  
  struct target {
    
    void construct(context& ctx_, frontend::target* target);
    void make(context& ctx_, frontend::target* target);
    void make_renderpass(context& ctx_, frontend::target* target);
    void make_framebuffer(context& ctx_, frontend::target* target);
    VkFramebuffer get_framebuffer(context& ctx_, frontend::target* target);
    VkRenderPass get_renderpass(context& ctx_, frontend::target* target);
    void start_renderpass(context& ctx_, frontend::target* target, VkCommandBuffer command);
    void end_renderpass(context& ctx_, frontend::target* target, VkCommandBuffer command);
    void destroy(context& ctx_, frontend::target* target);
    void clear(context& ctx_, frontend::target* target, const frontend::clear_command* clear);
    
    rx::array<VkFramebuffer[k_max_frames]> framebuffers;
    
    rx::vector<VkImageView> views;
    VkRenderPass renderpass = VK_NULL_HANDLE;
    
    rx::vector<bool> clears;
    
    rx_size num_attachments;
    
  };
  
  struct program {
    
    void construct(context& ctx_, frontend::program* program);
    void destroy(context& ctx_, frontend::program* program);
    
  };
  
  void pre_blit(context& ctx_, const frontend::blit_command* blit);
  void blit(context& ctx_, const frontend::blit_command* blit);
  
  void pre_draw(context& ctx_, const frontend::draw_command* draw);
  void draw(context& ctx_, const frontend::draw_command* draw);

}

}

#endif
