#ifndef RX_RENDER_BACKEND_VK_DATA_BUILDER_H
#define RX_RENDER_BACKEND_VK_DATA_BUILDER_H

#define VK_NO_PROTOTYPES
#include "vulkan/vulkan.h"

#include "helper.h"
#include "sync.h"

#include "rx/render/frontend/buffer.h"
#include "rx/render/frontend/target.h"
#include "rx/render/frontend/program.h"
#include "rx/render/frontend/resource.h"
#include "rx/render/frontend/texture.h"
#include "rx/render/frontend/command.h"

#include "rx/core/array.h"
#include "rx/core/map.h"

namespace rx::render::backend {

namespace detail_vk {
  
  struct context;
  
  struct buffer {
    
    buffer();
    
    void destroy(context& ctx_, frontend::buffer* buffer);
    
    use_queue::use_info* add_use(context& ctx_, VkPipelineStageFlags stage, VkAccessFlags access, uint32_t queue, bool write, bool sync_after = false);
    
    void sync(context& ctx_, frontend::buffer* buffer, const use_queue::use_info* last_use, VkCommandBuffer command);
    
    VkBuffer handle = VK_NULL_HANDLE;
    uint32_t offset;
    
#if defined(RX_DEBUG)
    const char* name = nullptr;
#endif
    
    use_queue::use_info last_use;
    
  };
  
  
  
  struct texture {
    
    texture();
    
    void construct_base(context& ctx_, frontend::texture* texture, VkImageCreateInfo& info);
    void update_base(context& ctx_, frontend::texture* texture);
    void get_size(context& ctx_, frontend::texture* texture);
    VkImageView make_attachment(context& ctx_, frontend::texture* texture, uint32_t layer, uint32_t level);
    void destroy(context& ctx_, frontend::texture* texture);
    
    use_queue::use_info* add_use(context& ctx_, VkImageLayout layout, VkPipelineStageFlags stage, VkAccessFlags access, uint32_t queue, bool write, bool sync_after = false);
    
    void sync(context& ctx_, frontend::texture* texture, const use_queue::use_info* last_use, VkCommandBuffer command);
    
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
    
    use_queue::use_info last_use;
    
  };
  
  
  
  class buffer_builder {
  public:
    struct buffer_info {
      const frontend::buffer* resource;
      const use_queue::use_info* use_info;
    };
    
    buffer_builder(context& ctx_);
    void construct(context& ctx_, frontend::buffer* buffer);
    void construct2(detail_vk::context& ctx_, buffer_info& info);
    void build(context& ctx_);
    
  private:
    rx::vector<buffer_info> buffer_infos;
    
    uint32_t current_buffer_size = 0;
    uint32_t buffer_type_bits = 0xffffffff;
    VkDeviceMemory buffer_memory = VK_NULL_HANDLE;
    VkDeviceMemory staging_memory = VK_NULL_HANDLE;
    VkBuffer staging_buffer = VK_NULL_HANDLE;
    rx_byte* buffer_staging_pointer = nullptr;
  };
  
  
  
  class texture_builder {
  public:
    struct texture_info {
      const frontend::resource_command* resource;
      const use_queue::use_info* use_info;
      VkDeviceSize staging_offset;
      VkDeviceSize bind_offset;
    };
    
    texture_builder(context& ctx_);
    void construct(context& ctx_, const frontend::resource_command* resource);
    void construct2(context& ctx_, texture_info& info);
    void build(context& ctx_);
    
  private:
    rx::vector<texture_info> texture_infos;
    
    VkDeviceSize current_image_size = 0;
    VkDeviceSize current_image_staging_size = 0;
    uint32_t image_type_bits = 0xffffffff;
    VkDeviceMemory image_memory = VK_NULL_HANDLE;
    VkDeviceMemory staging_memory = VK_NULL_HANDLE;
    VkBuffer staging_buffer = VK_NULL_HANDLE;
    rx_byte* image_staging_pointer = nullptr;
  };
  
}

}


#endif // RX_RENDER_BACKEND_VK_DATA_BUILDER_H
