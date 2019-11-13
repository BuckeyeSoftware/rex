#ifndef RX_RENDER_BACKEND_VK_PROGRAM_H
#define RX_RENDER_BACKEND_VK_PROGRAM_H

#define VK_NO_PROTOTYPES
#include "vulkan/vulkan.h"

#include "rx/render/frontend/program.h"

namespace rx::render::backend {

namespace detail_vk {
  
  struct context;
  
  struct program {
    
    void construct(context&, frontend::program*);
    void destroy(context&, frontend::program*);
    
  };

}

}

#endif
