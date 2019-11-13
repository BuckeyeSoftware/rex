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
    
    void destroy(context& ctx_, frontend::buffer* buffer);
    
    VkBuffer handle = VK_NULL_HANDLE;
    uint32_t offset;
    
  };
  
  
  
  struct texture {
    
    struct texture_use_queue {
      
      struct texture_use_info {
        texture_use_info(VkImageLayout required_layout, VkPipelineStageFlags use_stage, VkAccessFlags use_access, bool sync_before, bool sync_after)
        : required_layout(required_layout), use_stage(use_stage), use_access(use_access), sync_before(sync_before), sync_after(sync_after) {};
        VkImageLayout required_layout;
        VkPipelineStageFlags use_stage;
        VkAccessFlags use_access;
        bool sync_before;
        bool sync_after;
      };
      
      void push(VkImageLayout required_layout, bool sync_before = false, bool sync_after = false, VkPipelineStageFlags use_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VkAccessFlags use_access = 0) {
        RX_ASSERT(frame_use_index == 0, "adding texture use to a texture use queue that hasn't been cleared");
        frame_uses.emplace_back(required_layout, use_stage, use_access, sync_before, sync_after);
      }
      
      texture_use_info pop() {
        texture_use_info current = next();
        frame_use_index++;
        if(is_empty()) {
          frame_uses.clear();
          frame_use_index = 0;
        }
        return current;
      }
      
      texture_use_info& last() {
        RX_ASSERT(has_last(), "impossible to read last element of a texture use queue, doesn't exist");
        return frame_uses[frame_use_index-1];
      }
      
      texture_use_info& next() {
        RX_ASSERT(has_next(), "impossible to read next element of a texture use queue, doesn't exist");
        return frame_uses[frame_use_index];
      }
      
      bool has_last() {
        return frame_use_index>=1;
      }
      
      bool has_next() {
        return !is_empty();
      }
      
      bool is_empty() {
        return frame_use_index >= frame_uses.size();
      }
      
      rx::vector<texture_use_info> frame_uses;
      rx_size frame_use_index {0};
      
    };
    
    
    void construct_base(context& ctx_, frontend::texture* texture, VkImageCreateInfo& info);
    void update_base(context& ctx_, frontend::texture* texture);
    void get_size(context& ctx_, frontend::texture* texture);
    VkImageView make_attachment(context& ctx_, frontend::texture* texture, uint32_t layer, uint32_t level);
    void destroy(context& ctx_, frontend::texture* texture);
    
    void transfer_layout(context& ctx_, frontend::texture* texture, VkCommandBuffer command, texture_use_queue::texture_use_info use_info);
    
    VkImage handle {VK_NULL_HANDLE};
    VkFormat format;
    VkExtent3D extent;
    uint32_t offset;
    uint32_t layers;
    VkImageLayout current_layout;
    
    
    texture_use_queue frame_uses;
    
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
