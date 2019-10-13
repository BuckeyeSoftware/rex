#ifndef RX_RENDER_BACKEND_VK_HELPER_H
#define RX_RENDER_BACKEND_VK_HELPER_H

#define VK_NO_PROTOTYPES
#include "vulkan/vulkan.h"

#include "rx/core/vector.h"
#include "rx/core/log.h"

namespace rx::render::backend {
  
namespace detail_vk {
  struct context;
}

RX_LOG("render/vk", vk_log);

constexpr rx_size k_buffered = 2;

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

}

#endif // RX_RENDER_BACKEND_VK_HELPER_H
