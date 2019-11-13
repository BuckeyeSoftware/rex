#ifndef RX_RENDER_BACKEND_VK_CONTEXT_H
#define RX_RENDER_BACKEND_VK_CONTEXT_H

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#include "helper.h"

#include "rx/render/frontend/command.h"

#include <SDL.h>

#define LOCAL_INST_LOAD(_name) \
PFN_##_name _name = reinterpret_cast<PFN_##_name> (ctx_.vkGetInstanceProcAddr(ctx_.instance, #_name)); \
RX_ASSERT(_name, "can't load vulkan function pointer %s", #_name);

#define LOCAL_DEV_LOAD(_name) \
PFN_##_name _name = reinterpret_cast<PFN_##_name> (ctx_.vkGetDeviceProcAddr(ctx_.device, #_name)); \
RX_ASSERT(_name, "can't load vulkan function pointer %s", #_name);

namespace rx::render::backend {

#define DEV_FUN(_name) extern PFN_##_name _name;
#define INST_FUN(_name) extern PFN_##_name _name;
#include "prototypes.h"
#undef DEV_FUN
#undef INST_FUN

namespace detail_vk {
  
  struct texture;
  
  struct context {
    
    memory::allocator* allocator;
    SDL_Window* window;
    
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT callback;
    
    VkSurfaceKHR surface;
    
    VkPhysicalDevice physical;
    VkDevice device = VK_NULL_HANDLE;
    
    VkPhysicalDeviceMemoryProperties memory_properties;
    bool is_dedicated;
    
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr = nullptr;
    
    uint32_t graphics_index;
    VkQueue graphics_queue;
    
    struct allocation {
      VkDeviceMemory memory;
    };
    
    rx::vector<allocation> buffer_allocations;
    rx::vector<allocation> image_allocations;
    rx::vector<allocation> staging_allocations;
    rx::vector<VkBuffer> staging_allocation_buffers;
    
    struct {
    
      texture* image = nullptr;
      
      VkExtent2D extent;
      rx_size num_frames = k_max_frames;
      VkSwapchainKHR swapchain = VK_NULL_HANDLE;
      rx::array<VkImage[k_max_frames]> images;
      rx::array<VkImageView[k_max_frames]> image_views;
      uint32_t frame_index;
      bool acquired = false;
      
    } swap;
    
    VkSemaphore start_semaphore, end_semaphore;
    
    Command graphics;
    Command transfer;
    
    const frontend::command_header* current_command;
    
  };
  
}

}

#endif // RX_RENDER_BACKEND_VK_CONTEXT_H
