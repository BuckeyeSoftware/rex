#ifndef RX_RENDER_BACKEND_VK_DRAW_H
#define RX_RENDER_BACKEND_VK_DRAW_H

#define VK_NO_PROTOTYPES
#include "vulkan/vulkan.h"

#include "rx/render/frontend/command.h"
#include "rx/core/vector.h"
#include "sync.h"
#include "data_builder.h"

namespace rx::render::backend {

namespace detail_vk {
  
  struct context;
  
  frontend::texture* get_texture (const frontend::target::attachment& attachment);
  texture* get_tex (context& ctx_, const frontend::target::attachment& attachment);
  
  struct frame_render {
    
    struct renderpass_info {
        
      renderpass_info(context& ctx_, frontend::target* target);
      
      frontend::target* target;
      
      rx::vector<use_queue::use_info*> attachment_uses;
      use_queue::use_info* depth_stencil_use = nullptr;
      
      /*
      struct texture_use {
        
        use_queue::use_info info;
      };
      rx::vector<use_queue::use_info> texture_uses;
      */
        
      const frontend::clear_command* clear = nullptr;
      
      rx::vector<const frontend::draw_command*> draws;
      
      struct blit_info {
        const frontend::blit_command* blit;
        use_queue::use_info* src_use;
        use_queue::use_info* dst_use;
      };
      
      rx::vector<blit_info> blits;
      
    };
    
    rx::vector<frame_render::renderpass_info> renderpasses;
    
    frame_render(context& ctx_);
    
    void pre_sync(context& ctx_, const frontend::target* target);
  
    void pre_clear(context& ctx_, const frontend::clear_command* clear);
    
    void pre_draw(context& ctx_, const frontend::draw_command* draw);
    
    void pre_blit(context& ctx_, const frontend::blit_command* blit);
    
    void render(context& ctx_);
    
    
    void blit(context& ctx_, frame_render::renderpass_info::blit_info& blit_info);
    
    
  };

}

}

#endif
