#include "init.h"

#include "helper.h"

#include "context.h"

#include "data_builder.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#include "rx/core/log.h"
#include "rx/core/assert.h"
#include "rx/console/variable.h"
#include "rx/console/interface.h"
#include "rx/core/algorithm/clamp.h"

namespace rx::render::backend {
  
RX_CONSOLE_IVAR(
  num_frames,
  "display.swap_buffering",
  "number of buffer (1 = single buffer, 2 = double buffer, 3 = triple buffer)",
  1,
  3,
  2);
  
#define DEV_FUN(_name) PFN_##_name _name;
#define INST_FUN(_name) PFN_##_name _name;
#include "prototypes.h"
#undef DEV_FUN
#undef INST_FUN

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

bool create_instance(detail_vk::context& ctx_) {
  
  // load the only entry point function from loader
  *reinterpret_cast<void**> (&ctx_.vkGetInstanceProcAddr) = SDL_Vulkan_GetVkGetInstanceProcAddr();
  if(!ctx_.vkGetInstanceProcAddr) {
    vk_log(log::level::k_error, "could not load vulkan library");
    return false;
  }
  
  {
    // INSTANCE CREATION
    
    // fetch necessary extensions for windowing
    unsigned int count;
    if (!SDL_Vulkan_GetInstanceExtensions(ctx_.window, &count, nullptr)) {
      vk_log(log::level::k_error, SDL_GetError());
    }
    
    rx::vector<const char*> extensions(ctx_.allocator, count);
    
    if (!SDL_Vulkan_GetInstanceExtensions(ctx_.window, &count, extensions.data())) {
      vk_log(log::level::k_error, SDL_GetError());
    }
    
    
    rx::vector<const char*> layers(ctx_.allocator);
    
    // add debug extensions and validation layers
#if defined(RX_DEBUG)
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    layers.push_back("VK_LAYER_KHRONOS_validation");
#endif
    
    // check for extension support
    if(!check_instance_extensions(ctx_, extensions)) {
      vk_log(log::level::k_error, "missing necessary extension");
    }
    
    // check for extension support
    if(!check_layers(ctx_, layers)) {
      vk_log(log::level::k_info, "missing optional layers");
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
    check_result(vkCreateInstance(&createInfo, nullptr, &ctx_.instance));
    
    vk_log(log::level::k_info, "vulkan instance created");
    
  }
  
#if defined(RX_DEBUG)
  { // create debug callback
    VkDebugUtilsMessengerCreateInfoEXT info {};
    info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
    info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    info.pfnUserCallback = &debug_callback;

    LOCAL_INST_LOAD(vkCreateDebugUtilsMessengerEXT);
    check_result(vkCreateDebugUtilsMessengerEXT(ctx_.instance, &info, nullptr, &ctx_.callback));
  }
#endif
  
  if(SDL_Vulkan_CreateSurface(ctx_.window, ctx_.instance, &ctx_.surface) == SDL_FALSE) {
    vk_log(log::level::k_error, "SDL could not create vulkan surface : %s", SDL_GetError());
    return false;
  }
  
  return true;
  
}

void destroy_instance(detail_vk::context& ctx_) {
  
  LOCAL_INST_LOAD(vkDestroyDebugUtilsMessengerEXT);
  vkDestroyDebugUtilsMessengerEXT(ctx_.instance, ctx_.callback, nullptr);
  
  LOCAL_INST_LOAD(vkDestroyInstance)
  vkDestroyInstance(ctx_.instance, nullptr);
  
  vk_log(log::level::k_info, "vulkan instance destroyed");
  
}

void create_device(detail_vk::context& ctx_) {
  
  LOCAL_INST_LOAD(vkEnumeratePhysicalDevices)
  LOCAL_INST_LOAD(vkGetPhysicalDeviceProperties)
  LOCAL_INST_LOAD(vkGetPhysicalDeviceFeatures)
  LOCAL_INST_LOAD(vkEnumerateDeviceExtensionProperties)
  
  rx::vector<const char*> required_extensions(ctx_.allocator, 1, {VK_KHR_SWAPCHAIN_EXTENSION_NAME});
  VkPhysicalDeviceFeatures required_features {};
  int anisotropy = console::interface::get_from_name("gl4.anisotropy")->cast<int>()->get();
  if(anisotropy != 0) required_features.samplerAnisotropy = VK_TRUE;
  
  { // select physical device
    
    uint32_t count;
    vkEnumeratePhysicalDevices(ctx_.instance, &count, nullptr);
    
    rx::vector<VkPhysicalDevice> devices(ctx_.allocator, count);
    vkEnumeratePhysicalDevices(ctx_.instance, &count, devices.data());
    
    
    int max_index {-1};
    int index {0};
    int max_score{0};
    
    // test each device, and pick the best one
    devices.each_fwd([&](const VkPhysicalDevice device) {
      
      VkPhysicalDeviceProperties properties;
      vkGetPhysicalDeviceProperties(device, &properties);
      
      VkPhysicalDeviceFeatures features;
      vkGetPhysicalDeviceFeatures(device, &features);
      
      uint32_t count {0};
      vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
      rx::vector<VkExtensionProperties> extensions(ctx_.allocator, count);
      vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data());
      
      int score = 1;
      
      if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score++;
      }
      
      if (features.samplerAnisotropy == VK_TRUE) {
        score++;
      }
      
      if ( // test if required extensions are supported
        !required_extensions.each_fwd([&](const char* extension_name) -> bool {
          return !extensions.each_fwd([&](const VkExtensionProperties& extension) -> bool {
            return strcmp(extension_name, extension.extensionName);
          });
        })
      ) {
        score = 0;
      }
      
      if(score > max_score) {
        max_index = index;
        max_score = score;
      }
      index++;
      
    });
    
    RX_ASSERT(max_index>=0, "could not find a suitable vulkan device");
    
    ctx_.physical = devices[max_index];
    
  }
  
  {
    
    LOCAL_INST_LOAD(vkGetPhysicalDeviceMemoryProperties)
    vkGetPhysicalDeviceMemoryProperties(ctx_.physical, &ctx_.memory_properties);
    ctx_.is_dedicated = is_dedicated(ctx_);
    
  }
  
  {
    
    LOCAL_INST_LOAD(vkGetPhysicalDeviceQueueFamilyProperties)
    uint32_t count;
    vkGetPhysicalDeviceQueueFamilyProperties(ctx_.physical, &count, nullptr);
    rx::vector<VkQueueFamilyProperties> queue_families(ctx_.allocator, count);
    vkGetPhysicalDeviceQueueFamilyProperties(ctx_.physical, &count, queue_families.data());
    
    LOCAL_INST_LOAD(vkGetPhysicalDeviceSurfaceSupportKHR)
    
    rx_size index {0};
    bool found_queue = !queue_families.each_fwd([&](const VkQueueFamilyProperties& queue_family) -> bool {
      VkBool32 supports_present = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(ctx_.physical, index, ctx_.surface, &supports_present);
      if(queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT && supports_present) {
        return false;
      }
      index++;
      return true;
    });
    
    RX_ASSERT(found_queue, "could not find a vulkan graphics queue");
    
    ctx_.graphics_index = index;
    
  }
  
  {
    
    float priority = 1.0f;
    rx::vector<VkDeviceQueueCreateInfo> queue_info (ctx_.allocator, 1, {
      VkDeviceQueueCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueFamilyIndex = ctx_.graphics_index,
        .queueCount = 1,
        .pQueuePriorities = &priority
      }
    });
    
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(ctx_.physical, &features);
    if(anisotropy != 0) {
      if(features.samplerAnisotropy != VK_TRUE) {
        console::interface::get_from_name("gl4.anisotropy")->cast<int>()->set(0);
        required_features.samplerAnisotropy = VK_FALSE;
      }
    }
    
    
    VkDeviceCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    info.pEnabledFeatures = &required_features;
    info.enabledExtensionCount = required_extensions.size();
    info.ppEnabledExtensionNames = required_extensions.data();
    info.queueCreateInfoCount = queue_info.size();
    info.pQueueCreateInfos = queue_info.data();
    
    LOCAL_INST_LOAD(vkCreateDevice)
    check_result(vkCreateDevice(ctx_.physical, &info, nullptr, &ctx_.device));
    
    vk_log(log::level::k_info, "vulkan device created");
    
  }
  
  {
    LOCAL_INST_LOAD(vkGetDeviceProcAddr)
    ctx_.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
  }
  
  {
    
    LOCAL_DEV_LOAD(vkGetDeviceQueue)
    vkGetDeviceQueue(ctx_.device, ctx_.graphics_index, 0, &ctx_.graphics_queue);
    
  }
  
}

void destroy_device(detail_vk::context& ctx_) {
  
  LOCAL_DEV_LOAD(vkDestroyDevice)
  vkDestroyDevice(ctx_.device, nullptr);
  
  vk_log(log::level::k_info, "vulkan device destroyed");
  
}

void create_swapchain(detail_vk::context& ctx_) {
  
  {
    
    VkSwapchainCreateInfoKHR info {};
    info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.surface = ctx_.surface;
    
    {
      
      VkSurfaceCapabilitiesKHR surface_capabilities;
      
      check_result(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx_.physical, ctx_.surface, &surface_capabilities));
      
      if((int) surface_capabilities.minImageCount > num_frames->get() || (surface_capabilities.maxImageCount != 0 && (int) surface_capabilities.maxImageCount < num_frames->get())) {
        num_frames->set(rx::algorithm::clamp(num_frames->get(), (int) surface_capabilities.minImageCount, (int) surface_capabilities.maxImageCount));
        vk_log(log::level::k_warning, "number of buffers is not supported by the driver, changed to %i", num_frames->get());
      }
      
      info.minImageCount = num_frames->get();
      
      
      info.imageExtent = surface_capabilities.currentExtent;
      ctx_.swap.extent = info.imageExtent;
      
      vk_log(log::level::k_verbose, "swapchain extent : (%i, %i)", info.imageExtent.width, info.imageExtent.height);
      
      info.imageArrayLayers = 1;
      info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
      info.preTransform = surface_capabilities.currentTransform;
      info.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
      
      info.clipped = VK_TRUE;
      
    }
    
    
    {
      
      uint32_t count;
      check_result(vkGetPhysicalDeviceSurfaceFormatsKHR(ctx_.physical, ctx_.surface, &count, nullptr));
      rx::vector<VkSurfaceFormatKHR> surface_formats(ctx_.allocator, count);
      check_result(vkGetPhysicalDeviceSurfaceFormatsKHR(ctx_.physical, ctx_.surface, &count, surface_formats.data()));
      
      VkFormat target_format = VK_FORMAT_B8G8R8A8_SRGB;
      bool found = !surface_formats.each_fwd([&](const VkSurfaceFormatKHR& format) -> bool {
        if(format.format == target_format) {
          info.imageFormat = format.format;
          info.imageColorSpace = format.colorSpace;
          return false;
        }
        return true;
      });
      
      if(!found) {
        vk_log(log::level::k_error, "could not find a surface format with format : %d", target_format);
      }
      
      ctx_.swap.image->format = info.imageFormat;
      vk_log(log::level::k_verbose, "surface format : %d with color space : %d", info.imageFormat, info.imageColorSpace);
      
    }
    
    {
      
      uint32_t count;
      vkGetPhysicalDeviceSurfacePresentModesKHR(ctx_.physical, ctx_.surface, &count, nullptr);
      rx::vector<VkPresentModeKHR> present_modes(ctx_.allocator, count);
      vkGetPhysicalDeviceSurfacePresentModesKHR(ctx_.physical, ctx_.surface, &count, present_modes.data());
      
      VkPresentModeKHR target_mode = VK_PRESENT_MODE_FIFO_KHR;
      
      int interval = console::interface::get_from_name("display.swap_interval")->cast<int>()->get();
      switch (interval) {
        case 0:
          target_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
          break;
        case 1:
          target_mode = VK_PRESENT_MODE_FIFO_KHR;
          break;
        case -1:
          target_mode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
          break;
      }
      
      bool found = !present_modes.each_fwd([&](const VkPresentModeKHR& mode) -> bool {
        return !(mode == target_mode);
      });
      
      if(!found) {
        if(target_mode == VK_PRESENT_MODE_FIFO_KHR) {
          target_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        } else {
          target_mode = VK_PRESENT_MODE_FIFO_KHR;
        }
        vk_log(log::level::k_warning, "driver does not support this swap interval, defaulted to v-sync");
      }
      
      info.presentMode = target_mode;
      
    }
    
    info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.queueFamilyIndexCount = 1;
    info.pQueueFamilyIndices = &ctx_.graphics_index;
    
    VkSwapchainKHR old_swapchain = ctx_.swap.swapchain;
    info.oldSwapchain = old_swapchain;
    
    check_result(vkCreateSwapchainKHR(ctx_.device, &info, nullptr, &ctx_.swap.swapchain));
    
    vk_log(log::level::k_info, "vulkan swapchain created");
    
    if(old_swapchain != VK_NULL_HANDLE) {
      vkDestroySwapchainKHR(ctx_.device, old_swapchain, nullptr);
    }
    
    uint32_t count;
    vkGetSwapchainImagesKHR(ctx_.device, ctx_.swap.swapchain, &count, nullptr);
    ctx_.swap.num_frames = count;
    
    vkGetSwapchainImagesKHR(ctx_.device, ctx_.swap.swapchain, &count, ctx_.swap.images.data());
    
    ctx_.swap.image->current_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    
    ctx_.swap.image->extent.width = ctx_.swap.extent.width;
    ctx_.swap.image->extent.height = ctx_.swap.extent.height;
    ctx_.swap.image->extent.depth = 1;
    
    ctx_.swap.image->layers = info.imageArrayLayers;
    ctx_.swap.image->offset = 0;
    
    ctx_.swap.image->handle = VK_NULL_HANDLE;
    
  }
  
  for(rx_size i {0}; i<ctx_.swap.num_frames; i++) {
    
    VkImageViewCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.format = ctx_.swap.image->format;
    info.image = ctx_.swap.images[i];
    info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.baseMipLevel = 0;
    info.subresourceRange.layerCount = 1;
    info.subresourceRange.levelCount = 1;
    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    
    check_result(vkCreateImageView(ctx_.device, &info, nullptr, &ctx_.swap.image_views[i]));
    
  }
  
  {
    
    VkSemaphoreCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    vkCreateSemaphore(ctx_.device, &info, nullptr, &ctx_.start_semaphore);
    vkCreateSemaphore(ctx_.device, &info, nullptr, &ctx_.end_semaphore);
    
  }
  
}

void destroy_swapchain(detail_vk::context& ctx_) {
  
  vkDestroySemaphore(ctx_.device, ctx_.start_semaphore, nullptr);
  vkDestroySemaphore(ctx_.device, ctx_.end_semaphore, nullptr);
  
  for(rx_size i {0}; i<ctx_.swap.num_frames; i++) {
    vkDestroyImageView(ctx_.device, ctx_.swap.image_views[i], nullptr);
  }
  
  vkDestroySwapchainKHR(ctx_.device, ctx_.swap.swapchain, nullptr);
  
  vk_log(log::level::k_info, "vulkan swapchain destroyed");
  
}

void load_function_pointers(detail_vk::context& ctx_) {

#define DEV_FUN(_name) \
_name = reinterpret_cast<PFN_##_name> (ctx_.vkGetDeviceProcAddr(ctx_.device, #_name)); \
RX_ASSERT(_name, "can't load vulkan function pointer %s", #_name);

#define INST_FUN(_name) \
_name = reinterpret_cast<PFN_##_name> (ctx_.vkGetInstanceProcAddr(ctx_.instance, #_name)); \
RX_ASSERT(_name, "can't load vulkan function pointer %s", #_name);

#include "prototypes.h"

#undef INST_LOAD
#undef DEV_LOAD

}

}
