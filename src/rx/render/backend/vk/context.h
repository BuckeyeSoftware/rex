#ifndef RX_RENDER_BACKEND_VK_CONTEXT_H
#define RX_RENDER_BACKEND_VK_CONTEXT_H

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#include <SDL.h>

#include "rx/core/array.h"
#include "rx/core/log.h"

#define LOCAL_INST_LOAD(_name) \
PFN_##_name _name = reinterpret_cast<PFN_##_name> (ctx_.vkGetInstanceProcAddr(ctx_.instance, #_name)); \
RX_ASSERT(_name, "can't load vulkan function pointer %s", #_name);

#define INST_LOAD(_name) \
_name = reinterpret_cast<PFN_##_name> (ctx_.vkGetInstanceProcAddr(ctx_.instance, #_name)); \
RX_ASSERT(_name, "can't load vulkan function pointer %s", #_name);


#define LOCAL_DEV_LOAD(_name) \
PFN_##_name _name = reinterpret_cast<PFN_##_name> (ctx_.vkGetDeviceProcAddr(ctx_.device, #_name)); \
RX_ASSERT(_name, "can't load vulkan function pointer %s", #_name);

#define DEV_LOAD(_name) \
_name = reinterpret_cast<PFN_##_name> (ctx_.vkGetDeviceProcAddr(ctx_.device, #_name)); \
RX_ASSERT(_name, "can't load vulkan function pointer %s", #_name);

namespace rx::render::backend {

RX_LOG("render/vk", vk_log);

#define PFN(_name) extern PFN_##_name _name;
#include "prototypes.h"
#undef PFN

namespace detail_vk {
  
  constexpr rx_size buffered = 2;
  
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
    VkQueue graphics;
    
    struct allocation {
      VkDeviceMemory memory;
    };
    
    rx::vector<allocation> buffer_allocations;
    rx::vector<allocation> image_allocations;
    rx::vector<allocation> staging_allocations;
    
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    rx_size num_frames = 3;
    rx::array<VkImage[3]> swap_images;
    rx::array<VkImageView[3]> swap_image_view;
    
    VkCommandPool transfer_pool;
    rx::array<VkCommandBuffer[buffered]> transfer_commands;
    
  };
  
}

}

#endif // RX_RENDER_BACKEND_VK_CONTEXT_H
