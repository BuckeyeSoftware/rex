#ifndef RX_RENDER_BACKEND_VK_DATA_BUILDER_H
#define RX_RENDER_BACKEND_VK_DATA_BUILDER_H

#define VK_NO_PROTOTYPES
#include "vulkan/vulkan.h"

#include "helper.h"

#include "rx/render/frontend/buffer.h"
#include "rx/render/frontend/target.h"
#include "rx/render/frontend/program.h"
#include "rx/render/frontend/resource.h"
#include "rx/render/frontend/texture.h"
#include "rx/render/frontend/command.h"

#include "rx/core/array.h"

namespace rx::render::backend {

namespace detail_vk {
  
  struct context;
  
  struct use_queue {
      
    /**
      * Describes the use of a texture in a frame, the layout it needs, the stages during which it will be used, its access flags, it's family queue, and if it contains a write operation.
      * sync_after tells if this operation will synchronize after, instead of only before, by default, for example renderpasses synchronize after and before.
      */
    
    struct use_info {
      
      use_info(VkImageLayout layout, VkPipelineStageFlags stage, VkAccessFlags access, uint32_t queue, bool write, bool sync_after)
      : layout(layout), stage(stage), access(access), queue(queue), write(write), sync_after(sync_after) {};
      VkImageLayout layout;
      VkPipelineStageFlags stage;
      VkAccessFlags access;
      uint32_t queue;
      bool write;
      bool sync_after;
      rx_u32 counter = 1;
      
      use_info* after = nullptr;
      
    };
    
    void push(context& ctx_, VkImageLayout layout, VkPipelineStageFlags stage, VkAccessFlags access, uint32_t queue, bool write, bool sync_after);
    
    void pop(context& ctx_);
    
    use_info* current = nullptr;
    use_info* head = nullptr;
    
  };
  
  
  
  struct buffer {
    
    void destroy(context& ctx_, frontend::buffer* buffer);
    
    inline void add_use(context& ctx_, VkPipelineStageFlags stage, VkAccessFlags access, uint32_t queue, bool write, bool sync_after = false) {
      frame_uses.push(ctx_, VK_IMAGE_LAYOUT_UNDEFINED, stage, access, queue, write, sync_after);
    }
    void sync(context& ctx_, frontend::buffer* buffer, VkCommandBuffer command);
    
    VkBuffer handle = VK_NULL_HANDLE;
    uint32_t offset;
    
#if defined(RX_DEBUG)
    const char* name = nullptr;
#endif
    
  private:
    use_queue frame_uses;
    
  };
  
  
  
  struct texture {
    
    void construct_base(context& ctx_, frontend::texture* texture, VkImageCreateInfo& info);
    void update_base(context& ctx_, frontend::texture* texture);
    void get_size(context& ctx_, frontend::texture* texture);
    VkImageView make_attachment(context& ctx_, frontend::texture* texture, uint32_t layer, uint32_t level);
    void destroy(context& ctx_, frontend::texture* texture);
    
    inline void add_use(context& ctx_, VkImageLayout layout, VkPipelineStageFlags stage, VkAccessFlags access, uint32_t queue, bool write, bool sync_after = false) {
      frame_uses.push(ctx_, layout, stage, access, queue, write, sync_after);
    }
    void sync(context& ctx_, frontend::texture* texture, VkCommandBuffer command);
    
    VkImage handle {VK_NULL_HANDLE};
    VkFormat format;
    rx_size format_size;
    VkExtent3D extent;
    uint32_t offset;
    uint32_t layers;
    VkImageLayout current_layout;
    
#if defined(RX_DEBUG)
    const char* name = nullptr;
#endif
    
  private:
    use_queue frame_uses;
    
  };
  
  
  
  class buffer_builder {
  public:
    buffer_builder(context& ctx_);
    void construct(context& ctx_, frontend::buffer* buffer);
    
    void interpass(context& ctx_); 
    
    void construct2(context& ctx_, frontend::buffer* buffer);
    
  private:
    uint32_t current_buffer_size = 0;
    uint32_t buffer_type_bits = 0xffffffff;
    VkDeviceMemory buffer_memory = VK_NULL_HANDLE;
    VkDeviceMemory staging_memory = VK_NULL_HANDLE;
    VkBuffer staging_buffer = VK_NULL_HANDLE;
    rx_byte* buffer_staging_pointer = nullptr;
  };
  
  
  
  class texture_builder {
  public:
    texture_builder(context& ctx_);
    void construct(context& ctx_, const frontend::resource_command* resource);
    
    void interpass(context& ctx_);
    
    void construct2(context& ctx_, const frontend::resource_command* resource);
    
  private:
    uint32_t current_image_size = 0;
    uint32_t current_image_staging_size = 0;
    uint32_t image_type_bits = 0xffffffff;
    VkDeviceMemory image_memory = VK_NULL_HANDLE;
    VkDeviceMemory staging_memory = VK_NULL_HANDLE;
    VkBuffer staging_buffer = VK_NULL_HANDLE;
    rx_byte* image_staging_pointer = nullptr;
  };
  
}

}


#endif // RX_RENDER_BACKEND_VK_DATA_BUILDER_H
