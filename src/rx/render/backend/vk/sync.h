#ifndef RX_RENDER_BACKEND_VK_SYNC_H
#define RX_RENDER_BACKEND_VK_SYNC_H

#define VK_NO_PROTOTYPES
#include "vulkan/vulkan.h"

#include "rx/render/frontend/buffer.h"
#include "rx/render/frontend/target.h"
#include "rx/render/frontend/program.h"
#include "rx/render/frontend/resource.h"
#include "rx/render/frontend/texture.h"
#include "rx/render/frontend/command.h"

#include "rx/core/map.h"

namespace rx::render::backend {

namespace detail_vk {
  
  struct context;
  
  struct buffer;
  
  struct texture;
  
  struct use_queue {
    
    /**
      * Describes the use of a texture in a frame, the layout it needs, the stages during which it will be used, its access flags, it's family queue, and if it contains a write operation.
      * sync_after tells if this operation will synchronize after, instead of only before, by default, for example renderpasses synchronize after and before.
      */
    
    struct use_info {
      
      use_info() : counter(0) {};
      
      use_info(VkImageLayout layout, VkPipelineStageFlags stage, VkAccessFlags access, uint32_t queue, bool write, bool sync_after)
      : layout(layout), stage(stage), access(access), queue(queue), write(write), sync_after(sync_after), counter(1) {};
      VkImageLayout layout;
      VkPipelineStageFlags stage;
      VkAccessFlags access;
      uint32_t queue;
      bool write;
      bool sync_after;
      rx_u32 counter;
      
      use_info* after = nullptr;
      
    };
    
    use_queue(context& ctx_, use_info& first);
    
    use_queue::use_info* add_use(context& ctx_, VkImageLayout layout, VkPipelineStageFlags stage, VkAccessFlags access, uint32_t queue, bool write, bool sync_after);
    
    use_info clear(context& ctx_);
    
    use_info* tail = nullptr;
    use_info* head = nullptr;
    
  };
  
  struct resource_sync {
    resource_sync(context& ctx_);
    void clear(context& ctx_);
    rx::map<buffer*, use_queue> buffer_queues;
    rx::map<texture*, use_queue> texture_queues;
    use_queue& get_buffer_uses(context& ctx_, buffer* buffer);
    use_queue& get_texture_uses(context& ctx_, texture* texture);
  };
  
}

}

#endif
