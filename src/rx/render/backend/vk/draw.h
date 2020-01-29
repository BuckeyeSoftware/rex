#ifndef RX_RENDER_BACKEND_VK_DRAW_H
#define RX_RENDER_BACKEND_VK_DRAW_H

#define VK_NO_PROTOTYPES
#include "vulkan/vulkan.h"

#include "rx/render/frontend/command.h"
#include "rx/core/vector.h"
#include "sync.h"

namespace rx::render::backend {

namespace detail_vk {
  
  struct context;
  
  struct frame_render {
    
    frame_render(context& ctx_);
  
    void pre_clear(context& ctx_, const frontend::clear_command* clear);
    
    void pre_blit(context& ctx_, const frontend::blit_command* blit);
    
    void pre_draw(context& ctx_, const frontend::draw_command* draw);
    
    void render(context& ctx_);
    
    struct renderpass_info {
        
      renderpass_info(context& ctx_, frontend::target* target);
      
      frontend::target* target;
      
      rx::vector<use_queue::use_info> attachment_uses;
      
      /*
      struct texture_use {
        
        use_queue::use_info info;
      };
      rx::vector<use_queue::use_info> texture_uses;
      */
        
      const frontend::clear_command* clear;
      
      rx::vector<const frontend::draw_command*> draws;
      
      rx::vector<const frontend::blit_command*> blits;
      
    };
    
    rx_size renderpass_index {0};
    rx::vector<renderpass_info> renderpasses;
  
  };

}

}

#endif
