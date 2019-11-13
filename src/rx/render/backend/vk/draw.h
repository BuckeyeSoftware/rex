#ifndef RX_RENDER_BACKEND_VK_DRAW_H
#define RX_RENDER_BACKEND_VK_DRAW_H

#define VK_NO_PROTOTYPES
#include "vulkan/vulkan.h"

#include "rx/render/frontend/command.h"

namespace rx::render::backend {

namespace detail_vk {
  
  struct context;
  
  struct frame_render {
  
    void pre_clear(context& ctx_, const frontend::clear_command* clear);
    void clear(context& ctx_, const frontend::clear_command* clear);
    
    void pre_blit(context& ctx_, const frontend::blit_command* blit);
    void blit(context& ctx_, const frontend::blit_command* blit);
    
    void pre_draw(context& ctx_, const frontend::draw_command* draw);
    void draw(context& ctx_, const frontend::draw_command* draw);
  
  };

}

}

#endif
