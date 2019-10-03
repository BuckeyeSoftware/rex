#include "rx/render/backend/vk.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#include "rx/core/memory/allocator.h"

#include "rx/core/log.h"

#include "rx/core/assert.h"

RX_LOG("render/vk", vk_log);


#define LOCAL_INST_LOAD(_name) \
PFN_##_name _name = (PFN_##_name) ( ctx.vkGetInstanceProcAddr(ctx.instance, #_name)); \
RX_ASSERT(_name, "can't load vulkan function pointer %s", #_name);

#define INST_LOAD(_instance, _name) \
_name = (PFN_##_name) ( vkGetInstanceProcAddr(_instance, #_name)); \
RX_ASSERT(_name, "can't load vulkan function pointer %s", #_name);



namespace rx::render::backend {

namespace detail_vk {
  
  struct context {
    
    memory::allocator* allocator;
    SDL_Window* window;
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;
    VkInstance instance = nullptr;
    VkDebugUtilsMessengerEXT callback;
    
  };
  
}

bool check_layers(detail_vk::context& ctx, rx::vector<const char *>& layerNames) {
  
  LOCAL_INST_LOAD(vkEnumerateInstanceLayerProperties);
  uint32_t count;
  vkEnumerateInstanceLayerProperties(&count, nullptr);
  
  rx::vector<VkLayerProperties> availableLayers(ctx.allocator, count);
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

bool check_instance_extensions(detail_vk::context& ctx, rx::vector<const char *> extensionNames) {
    
  LOCAL_INST_LOAD(vkEnumerateInstanceExtensionProperties);
  uint32_t count;
  vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
  
  rx::vector<VkExtensionProperties> availableExtensions(ctx.allocator, count);
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


static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback (
  VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
  VkDebugUtilsMessageTypeFlagsEXT messageType,
  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
  void* pUserData) {
  
  log::level level;
  
  if(messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
    level = log::level::k_verbose;
  } else if(messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
    level = log::level::k_info;
  } else if(messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    level = log::level::k_warning;
  } else if(messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
    level = log::level::k_error;
  } else {
    level = log::level::k_info;
  }
  
  const char* prefix;
  if(messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
    prefix = "General";
  } else if(messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
    prefix = "Performance";
  } else if(messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
    prefix = "Validation";
  } else {
    prefix = "";
  }
  
  vk_log(level, "%s : %s", prefix, pCallbackData->pMessage);
  
  return VK_FALSE;
  
}

#ifndef NDEBUG
inline void check_result(VkResult result) {
  if(result != VK_SUCCESS) {
    vk_log(log::level::k_error, "vulkan call failed with result %i", result);
  }
}
#else
inline void check_result(VkResult result) {
  
}
#endif

  
vk::vk(memory::allocator* _allocator, void* _data) {
  
  m_impl = _allocator->create<detail_vk::context>();
  detail_vk::context& ctx{*reinterpret_cast<detail_vk::context*> (m_impl)};
  
  ctx.allocator = _allocator;
  ctx.window = reinterpret_cast<SDL_Window*>(_data);
  
  
  // load the only entry point function from loader
  ctx.vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr) SDL_Vulkan_GetVkGetInstanceProcAddr();
  if(!ctx.vkGetInstanceProcAddr) {
    vk_log(log::level::k_error, "could not load vulkan library");
  }
  
  
  {
    // INSTANCE CREATION
    
    // fetch necessary extensions for windowing
    unsigned int count;
    if (!SDL_Vulkan_GetInstanceExtensions(ctx.window, &count, nullptr)) {
      vk_log(log::level::k_error, SDL_GetError());
    }
    
    rx::vector<const char*> extensions(ctx.allocator, count);
    
    if (!SDL_Vulkan_GetInstanceExtensions(ctx.window, &count, extensions.data())) {
      vk_log(log::level::k_error, SDL_GetError());
    }
    
    
    rx::vector<const char*> layers(ctx.allocator);
    
    // add debug extensions and validation layers
  #ifndef NDEBUG
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    layers.push_back("VK_LAYER_KHRONOS_validation");
  #endif
    
    // check for extension support
    if(!check_instance_extensions(ctx, extensions)) {
      vk_log(log::level::k_error, "missing necessary extension");
    }
    
    // check for extension support
    if(!check_layers(ctx, layers)) {
      vk_log(log::level::k_warning, "missing optional layers");
      layers.clear();
    }
    
    // application info
    
    VkApplicationInfo appInfo {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pApplicationName = "Rex";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Rex";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    
    // create instance
    VkInstanceCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
    createInfo.ppEnabledLayerNames = layers.data();
    createInfo.pApplicationInfo = &appInfo;
    
    LOCAL_INST_LOAD(vkCreateInstance)
    check_result(vkCreateInstance(&createInfo, nullptr, &ctx.instance));
  }
  
#ifndef NDEBUG
  { // create debug callback
    VkDebugUtilsMessengerCreateInfoEXT info {};
    info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
    info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    info.pfnUserCallback = &debug_callback;

    LOCAL_INST_LOAD(vkCreateDebugUtilsMessengerEXT);
    check_result(vkCreateDebugUtilsMessengerEXT(ctx.instance, &info, nullptr, &ctx.callback));
  }
#endif
  
}

vk::~vk() {
  
  detail_vk::context& ctx{*reinterpret_cast<detail_vk::context*> (m_impl)};
  
  LOCAL_INST_LOAD(vkDestroyDebugUtilsMessengerEXT);
  vkDestroyDebugUtilsMessengerEXT(ctx.instance, ctx.callback, nullptr);
  
  LOCAL_INST_LOAD(vkDestroyInstance)
  vkDestroyInstance(ctx.instance, nullptr);
  
  ctx.allocator->destroy<detail_vk::context>(m_impl);
  
}

void vk::process(rx_byte* _command) {
  
}

void vk::swap() {
  
}

allocation_info vk::query_allocation_info() const {
  allocation_info info = {};
  return info;
}

device_info vk::query_device_info() const {
  device_info info = {};
  info.renderer = "";
  info.vendor = "";
  info.version = "";
  return info;
}

}
