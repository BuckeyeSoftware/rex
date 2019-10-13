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
  
  struct buffer {
    
    buffer(context& ctx_, frontend::buffer* buffer);
    void destroy(context& ctx_, frontend::buffer* buffer);
    
    VkBuffer handle = VK_NULL_HANDLE;
    uint32_t offset;
    
  };
  
  struct target {
    
    target(context& ctx_, frontend::target* target);
    void destroy(context& ctx_, frontend::target* target);
    
  };
  
  struct program {
    
    program(context& ctx_, frontend::program* program);
    void destroy(context& ctx_, frontend::program* program);
    
  };
  
  struct texture {
    
    texture(context& ctx_, frontend::texture* texture);
    void construct_base(context& ctx_, frontend::texture* texture, VkImageCreateInfo& info);
    void construct_base_view(context& ctx_, frontend::texture* texture, VkImageViewCreateInfo& info);
    void update_base(context& ctx_, frontend::texture* texture);
    void get_size(context& ctx_, frontend::texture* texture);
    void destroy(context& ctx_, frontend::texture* texture);
    
    VkImage handle = VK_NULL_HANDLE;
    VkFormat format;
    VkExtent3D extent;
    uint32_t offset;
    VkImageView view = VK_NULL_HANDLE;
    
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
  
  class Transfer {
  public:
    void init(context& ctx_);
    void start(context& ctx_);
    void end(context& ctx_);
    void destroy(context& ctx_);
    
    VkCommandBuffer get();
    
  private:
    
    VkCommandPool pool;
    rx::array<VkCommandBuffer[k_buffered]> commands;
    rx::array<VkFence[k_buffered]> fences;
    uint32_t index = 0;
    bool written = true;
  };
  
}

}


#endif // RX_RENDER_BACKEND_VK_DATA_BUILDER_H
