#include "helper.h"

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

}
