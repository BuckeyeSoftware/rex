#include "helper.h"

#include "context.h"

namespace rx::render::backend {

// helper functions

bool check_layers(detail_vk::context& ctx_, rx::vector<const char *>& layerNames) {
  
  LOCAL_INST_LOAD(vkEnumerateInstanceLayerProperties);
  uint32_t count;
  vkEnumerateInstanceLayerProperties(&count, nullptr);
  
  rx::vector<VkLayerProperties> availableLayers(ctx_.allocator, count);
  vkEnumerateInstanceLayerProperties(&count, availableLayers.data());
  
  return layerNames.each_fwd([&](const char* layerName) -> bool {
    
    if (
      availableLayers.each_fwd([&](const VkLayerProperties& layerProperties) {
        return strcmp(layerName, layerProperties.layerName) == 0;
      })
    ) {
      vk_log(log::level::k_error, "could not find layer %s", layerName);
      return false;
    }
    return true;
    
  });
  
}

bool check_instance_extensions(detail_vk::context& ctx_, rx::vector<const char *> extensionNames) {
  
  LOCAL_INST_LOAD(vkEnumerateInstanceExtensionProperties);
  uint32_t count;
  vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
  
  rx::vector<VkExtensionProperties> availableExtensions(ctx_.allocator, count);
  vkEnumerateInstanceExtensionProperties(nullptr, &count, availableExtensions.data());
  
  return extensionNames.each_fwd([&](const char* extensionName) {
  
    if (
      availableExtensions.each_fwd([&](const VkExtensionProperties& extensionProperties) -> bool {
        return strcmp(extensionName, extensionProperties.extensionName) == 0;
      })
    ) {
      vk_log(log::level::k_error, "could not find layer %s", extensionName);
      return false;
    }
    return true;
    
  });
    
}

uint32_t get_memory_type(detail_vk::context& ctx_, uint32_t typeBits, VkMemoryPropertyFlags properties) {
    for(uint32_t i = 0; i < ctx_.memory_properties.memoryTypeCount; i++) {
        if((typeBits & 1) == 1) {
            if((ctx_.memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        typeBits >>= 1;
    }
    return 1000;
}

bool is_dedicated(detail_vk::context& ctx_) {
  for(uint32_t i = 0; i < ctx_.memory_properties.memoryTypeCount; i++) {
    if(
      (ctx_.memory_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT &&
      (ctx_.memory_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
    ) {return true;}
  }
  return false;
}



#if defined(RX_DEBUG)
void detail_vk::set_name(context& ctx_, VkObjectType type, uint64_t handle, const char* name) {
  VkDebugUtilsObjectNameInfoEXT info {};
  info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
  info.objectType = type;
  info.objectHandle = handle;
  info.pObjectName = name;
  vkSetDebugUtilsObjectNameEXT(ctx_.device, &info);
}
#endif






void detail_vk::Command::init(detail_vk::context& ctx_, uint32_t queue_family) {
  
  {
    VkCommandPoolCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    info.queueFamilyIndex = queue_family;
    
    vkCreateCommandPool(ctx_.device, &info, nullptr, &pool);
  }
  
  {
    VkCommandBufferAllocateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.commandPool = pool;
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    info.commandBufferCount = commands.size();
    
    vkAllocateCommandBuffers(ctx_.device, &info, commands.data());
  }
  
  {
    VkFenceCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    //info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    for (rx_size i {0}; i<fences.size(); i++) {
      vkCreateFence(ctx_.device, &info, nullptr, &fences[i]);
    }
  }
  
}

void detail_vk::Command::start(detail_vk::context& ctx_) {
  
  if(!written) return;
  written = false;
  
  VkCommandBufferBeginInfo info {};
  info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  //info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  check_result(vkBeginCommandBuffer(commands[index], &info));
  
}

void detail_vk::Command::end(detail_vk::context& ctx_, VkQueue queue) {
  
  if (!written) return;
  
  VkCommandBuffer command = commands[index];
  check_result(vkEndCommandBuffer(command));
  
  VkSubmitInfo info {};
  info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  info.commandBufferCount = 1;
  info.pCommandBuffers = &command;
  check_result(vkQueueSubmit(queue, 1, &info, fences[index]));
  
  check_result(vkWaitForFences(ctx_.device, 1, &fences[index], VK_TRUE, 10000000000000L));
  
  check_result(vkResetFences(ctx_.device, 1, &fences[index]));
  
}

void detail_vk::Command::destroy(detail_vk::context& ctx_) {
  
  for (rx_size i {0}; i<fences.size(); i++) {
    vkDestroyFence(ctx_.device, fences[i], nullptr);
  }
  
  vkDestroyCommandPool(ctx_.device, pool, nullptr);
  
}

VkCommandBuffer detail_vk::Command::get() {
  
  written = true;
  return commands[index];
  
}

}
