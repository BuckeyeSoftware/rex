#ifndef RX_RENDER_BACKEND_VK_HELPER_H
#define RX_RENDER_BACKEND_VK_HELPER_H

#define VK_NO_PROTOTYPES
#include "vulkan/vulkan.h"

#include "rx/core/vector.h"
#include "rx/core/array.h"
#include "rx/core/log.h"

namespace rx::render::backend {
  
namespace detail_vk {
  struct context;
}

RX_LOG("render/vk", vk_log);

constexpr rx_size k_buffered = 2;
constexpr rx_size k_max_frames = 3;

// helper functions
bool check_layers(detail_vk::context& ctx_, rx::vector<const char *>& layerNames);
bool check_instance_extensions(detail_vk::context& ctx_, rx::vector<const char *> extensionNames);
uint32_t get_memory_type(detail_vk::context& ctx_, uint32_t typeBits, VkMemoryPropertyFlags properties);
bool is_dedicated(detail_vk::context& ctx_);

#if defined(RX_DEBUG)
inline void check_result(VkResult result) {
  if(result != VK_SUCCESS) {
    vk_log(log::level::k_error, "vulkan call failed with result %i", result);
  }
}
#else
inline void check_result(VkResult result) {
  (void)result;
}
#endif

#if defined(RX_DEBUG)
#define SET_NAME(CTX, TYPE, HANDLE, NAME) \
set_name(CTX, TYPE, (uint64_t) HANDLE, NAME)
#else
#define SET_NAME(CTX, TYPE, HANDLE, NAME)
#endif

// printer functions
const char* layout_to_string(VkImageLayout layout);

namespace detail_vk {
  
  class Command {
  public:
    void init(context& ctx_, uint32_t queue_family);
    void start(context& ctx_);
    void end(context& ctx_, VkQueue queue, rx::vector<VkSemaphore> wait, rx::vector<VkPipelineStageFlags> stage, rx::vector<VkSemaphore> signal);
    void destroy(context& ctx_);
    
    VkCommandBuffer get(context& ctx_);
    
  private:
    
    VkCommandPool pool;
    rx::array<VkCommandBuffer[k_buffered]> commands;
    rx::array<VkFence[k_buffered]> fences;
  };
  
  #if defined(RX_DEBUG)
  void set_name(context& ctx_, VkObjectType type, uint64_t handle, const char* name);
  #else
  inline void set_name(context&, VkObjectType, uint64_t, const char*) {}
  #endif
  
}

}

#endif // RX_RENDER_BACKEND_VK_HELPER_H
