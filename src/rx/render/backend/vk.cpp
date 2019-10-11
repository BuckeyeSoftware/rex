#include "rx/render/backend/vk.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#include "rx/core/memory/allocator.h"
#include "rx/core/vector.h"
#include "rx/core/log.h"
#include "rx/core/assert.h"
#include "rx/console/variable.h"
#include "rx/console/interface.h"
#include "rx/core/algorithm/clamp.h"
#include "rx/core/function.h"
#include "rx/core/math/ceil.h"

#include "rx/render/frontend/buffer.h"
#include "rx/render/frontend/target.h"
#include "rx/render/frontend/program.h"
#include "rx/render/frontend/state.h"
#include "rx/render/frontend/texture.h"
#include "rx/render/frontend/command.h"


RX_LOG("render/vk", vk_log);

RX_CONSOLE_IVAR(
  num_frames,
  "display.swap_buffering",
  "number of buffer (1 = single buffer, 2 = double buffer, 3 = triple buffer)",
  1,
  3,
  2);


#define PFN(_name) \
static PFN_##_name _name;

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


PFN(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
PFN(vkGetPhysicalDeviceSurfaceFormatsKHR)
PFN(vkGetPhysicalDeviceSurfacePresentModesKHR)
PFN(vkCreateSwapchainKHR)
PFN(vkDestroySwapchainKHR)

PFN(vkCreateBuffer)
PFN(vkDestroyBuffer)
PFN(vkCreateImage)
PFN(vkDestroyImage)
PFN(vkCreateImageView)
PFN(vkDestroyImageView)
PFN(vkGetBufferMemoryRequirements)
PFN(vkGetImageMemoryRequirements)
PFN(vkBindBufferMemory)
PFN(vkBindImageMemory)
PFN(vkAllocateMemory)
PFN(vkFreeMemory)
PFN(vkMapMemory)
PFN(vkUnmapMemory)


namespace rx::render::backend {

namespace detail_vk {
  
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
    rx::vector<VkImage> swap_images;
    rx::vector<VkImageView> swap_image_view;
    
  };
  
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
    rx_byte* image_staging_pointer = nullptr;
  };
  
}








// helper functions
bool check_layers(detail_vk::context& ctx_, rx::vector<const char *>& layerNames);
bool check_instance_extensions(detail_vk::context& ctx_, rx::vector<const char *> extensionNames);
uint32_t get_memory_type(detail_vk::context& ctx_, uint32_t typeBits, VkMemoryPropertyFlags properties);
bool is_dedicated(detail_vk::context& ctx_);



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

#if defined(RX_DEBUG)
inline void check_result(VkResult result) {
  if(result != VK_SUCCESS) {
    vk_log(log::level::k_error, "vulkan call failed with result %i", result);
  }
}
#else
inline void check_result(VkResult result) {
  
}
#endif


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
    vkGetDeviceQueue(ctx_.device, ctx_.graphics_index, 0, &ctx_.graphics);
    
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
      
      vk_log(log::level::k_verbose, "swapchain extent : (%i, %i)", info.imageExtent.width, info.imageExtent.height);
      
      info.imageArrayLayers = 1;
      info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
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
    
    VkSwapchainKHR old_swapchain = ctx_.swapchain;
    info.oldSwapchain = old_swapchain;
    
    check_result(vkCreateSwapchainKHR(ctx_.device, &info, nullptr, &ctx_.swapchain));
    
    vk_log(log::level::k_info, "vulkan swapchain created");
    
    if(old_swapchain != VK_NULL_HANDLE) {
      vkDestroySwapchainKHR(ctx_.device, old_swapchain, nullptr);
    }
    
  }
  
}

void destroy_swapchain(detail_vk::context& ctx_) {
  
  vkDestroySwapchainKHR(ctx_.device, ctx_.swapchain, nullptr);
  
  vk_log(log::level::k_info, "vulkan swapchain destroyed");
  
}

void load_function_pointers(detail_vk::context& ctx_) {
  
  INST_LOAD(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
  INST_LOAD(vkGetPhysicalDeviceSurfacePresentModesKHR)
  INST_LOAD(vkGetPhysicalDeviceSurfaceFormatsKHR)
  DEV_LOAD(vkCreateSwapchainKHR)
  DEV_LOAD(vkDestroySwapchainKHR)
  
  DEV_LOAD(vkCreateBuffer)
  DEV_LOAD(vkDestroyBuffer)
  DEV_LOAD(vkCreateImage)
  DEV_LOAD(vkDestroyImage)
  DEV_LOAD(vkCreateImageView)
  DEV_LOAD(vkDestroyImageView)
  DEV_LOAD(vkGetBufferMemoryRequirements)
  DEV_LOAD(vkGetImageMemoryRequirements)
  DEV_LOAD(vkBindBufferMemory)
  DEV_LOAD(vkBindImageMemory)
  DEV_LOAD(vkAllocateMemory)
  DEV_LOAD(vkFreeMemory)
  DEV_LOAD(vkMapMemory)
  DEV_LOAD(vkUnmapMemory)
  
}







vk::vk(memory::allocator* _allocator, void* _data) {
  
  m_impl = _allocator->create<detail_vk::context>();
  detail_vk::context& ctx{*reinterpret_cast<detail_vk::context*> (m_impl)};
  
  ctx.allocator = _allocator;
  ctx.window = reinterpret_cast<SDL_Window*>(_data);
  
  ctx.buffer_allocations = {ctx.allocator};
  ctx.image_allocations = {ctx.allocator};
  ctx.staging_allocations = {ctx.allocator};
  
}


bool vk::init() {
  
  detail_vk::context& ctx{*reinterpret_cast<detail_vk::context*> (m_impl)};
  
  if (!create_instance(ctx)) return false;
  
  create_device(ctx);
  
  load_function_pointers(ctx);
  
  return true;
  
}

vk::~vk() {
  
  detail_vk::context& ctx_{*reinterpret_cast<detail_vk::context*> (m_impl)};
  
  vk_log(log::level::k_info, "destroying vulkan backend");
  
  LOCAL_DEV_LOAD(vkDeviceWaitIdle)
  vkDeviceWaitIdle(ctx_.device);
  
  
  ctx_.buffer_allocations.each_fwd([&](detail_vk::context::allocation& alloc) {
    vkFreeMemory(ctx_.device, alloc.memory, nullptr);
  });
  ctx_.image_allocations.each_fwd([&](detail_vk::context::allocation& alloc) {
    vkFreeMemory(ctx_.device, alloc.memory, nullptr);
  });
  ctx_.staging_allocations.each_fwd([&](detail_vk::context::allocation& alloc) {
    vkFreeMemory(ctx_.device, alloc.memory, nullptr);
  });
  
  destroy_device(ctx_);
  
  destroy_instance(ctx_);
  
  ctx_.allocator->destroy<detail_vk::context>(m_impl);
  
}

void vk::process(const vector<rx_byte*>& _commands) {
  
  detail_vk::context& ctx{*reinterpret_cast<detail_vk::context*> (m_impl)};
  
  detail_vk::buffer_builder b_builder(ctx);
  detail_vk::texture_builder t_builder(ctx);
  
  
  // pre process
  _commands.each_fwd([&](rx_byte* _command) {
    
    auto header {reinterpret_cast<frontend::command_header*>(_command)};
    switch (header->type) {
      
      case frontend::command_type::k_resource_allocate: {
        const auto resource{reinterpret_cast<const frontend::resource_command*>(header + 1)};
        
        switch (resource->kind) {
          case frontend::resource_command::type::k_buffer:
            utility::construct<detail_vk::buffer>(resource->as_buffer + 1, ctx, resource->as_buffer);
            break;
          case frontend::resource_command::type::k_target:
            utility::construct<detail_vk::target>(resource->as_target + 1, ctx, resource->as_target);
            break;
          case frontend::resource_command::type::k_program:
            utility::construct<detail_vk::program>(resource->as_program + 1, ctx, resource->as_program);
            break;
          case frontend::resource_command::type::k_texture1D:
            utility::construct<detail_vk::texture>(resource->as_texture1D + 1, ctx, resource->as_texture1D);
            break;
          case frontend::resource_command::type::k_texture2D:
            utility::construct<detail_vk::texture>(resource->as_texture2D + 1, ctx, resource->as_texture2D);
            break;
          case frontend::resource_command::type::k_texture3D:
            utility::construct<detail_vk::texture>(resource->as_texture3D + 1, ctx, resource->as_texture3D);
            break;
          case frontend::resource_command::type::k_textureCM:
            utility::construct<detail_vk::texture>(resource->as_textureCM + 1, ctx, resource->as_textureCM);
            break;
        }
        break;
      }
      case frontend::command_type::k_resource_destroy: {
        break;
      }
      case frontend::command_type::k_resource_construct: {
        const auto resource{reinterpret_cast<const frontend::resource_command*>(header + 1)};
        
        switch (resource->kind) {
          case frontend::resource_command::type::k_buffer: {
            b_builder.construct(ctx, resource->as_buffer);
            break;
          }
          case frontend::resource_command::type::k_target: {
            auto target = resource->as_target;
            if(target->is_swapchain()) {
              create_swapchain(ctx);
            }
            break;
          }
          case frontend::resource_command::type::k_program:
            break;
          default:
            t_builder.construct(ctx, resource);
        }
        break;
      }
      case frontend::command_type::k_resource_update: {
        const auto resource{reinterpret_cast<const frontend::resource_command*>(header + 1)};
        
        switch (resource->kind) {
          case frontend::resource_command::type::k_buffer:
            break;
          case frontend::resource_command::type::k_target:
            break;
          case frontend::resource_command::type::k_program:
            break;
          case frontend::resource_command::type::k_texture1D:
            break;
          case frontend::resource_command::type::k_texture2D:
            break;
          case frontend::resource_command::type::k_texture3D:
            break;
          case frontend::resource_command::type::k_textureCM:
            break;
        }
        break;
      }
      case frontend::command_type::k_draw: {
        const auto draw {reinterpret_cast<const frontend::draw_command*>(header + 1)};
        
        break;
      }
      case frontend::command_type::k_clear: {
        const auto clear {reinterpret_cast<const frontend::clear_command*>(header + 1)};
        
        break;
      }
      case frontend::command_type::k_blit: {
        
        const auto blit {reinterpret_cast<const frontend::blit_command*>(header + 1)};
        break;
      }
      case frontend::command_type::k_profile: {
        const auto profile {reinterpret_cast<const frontend::profile_command*>(header + 1)};
        
        break;
      }
    }
    
  });
  
  // end first pass
  
  
  // allocate memory
  b_builder.interpass(ctx);
  t_builder.interpass(ctx);
  
  
  
  // main process
  
  _commands.each_fwd([&](rx_byte* _command) {
    
    auto header {reinterpret_cast<frontend::command_header*>(_command)};
    switch(header->type) {
      
      case frontend::command_type::k_resource_allocate: {
        break;
      }
      case frontend::command_type::k_resource_destroy: {
        const auto resource{reinterpret_cast<const frontend::resource_command*>(header + 1)};
        
        switch (resource->kind) {
          case frontend::resource_command::type::k_buffer:
            reinterpret_cast<detail_vk::buffer*>(resource->as_buffer + 1)->destroy(ctx, resource->as_buffer);
            utility::destruct<detail_vk::buffer>(resource->as_buffer + 1);
            break;
          case frontend::resource_command::type::k_target: {
            auto target = resource->as_target;
            if(target->is_swapchain()) {
              destroy_swapchain(ctx);
            }
            utility::destruct<detail_vk::target>(resource->as_target + 1);
            break;
          }
          case frontend::resource_command::type::k_program:
            utility::destruct<detail_vk::program>(resource->as_program + 1);
            break;
          case frontend::resource_command::type::k_texture1D:
            reinterpret_cast<detail_vk::texture*>(resource->as_texture1D + 1)->destroy(ctx, resource->as_texture1D);
            utility::destruct<detail_vk::texture>(resource->as_texture1D + 1);
            break;
          case frontend::resource_command::type::k_texture2D:
            reinterpret_cast<detail_vk::texture*>(resource->as_texture2D + 1)->destroy(ctx, resource->as_texture2D);
            utility::destruct<detail_vk::texture>(resource->as_texture2D + 1);
            break;
          case frontend::resource_command::type::k_texture3D:
            reinterpret_cast<detail_vk::texture*>(resource->as_texture3D + 1)->destroy(ctx, resource->as_texture3D);
            utility::destruct<detail_vk::texture>(resource->as_texture3D + 1);
            break;
          case frontend::resource_command::type::k_textureCM:
            reinterpret_cast<detail_vk::texture*>(resource->as_textureCM + 1)->destroy(ctx, resource->as_textureCM);
            utility::destruct<detail_vk::texture>(resource->as_textureCM + 1);
            break;
        }
        break;
      }
      case frontend::command_type::k_resource_construct: {
        const auto resource{reinterpret_cast<const frontend::resource_command*>(header + 1)};
        
        switch (resource->kind) {
          case frontend::resource_command::type::k_buffer: {
            b_builder.construct2(ctx, resource->as_buffer);
            break;
          }
          case frontend::resource_command::type::k_target:
            
            break;
          case frontend::resource_command::type::k_program:
            
            break;
          default: {
            t_builder.construct2(ctx, resource);
            break;
          }
        }
        break;
      }
      case frontend::command_type::k_resource_update: {
        const auto resource{reinterpret_cast<const frontend::resource_command*>(header + 1)};
        
        switch (resource->kind) {
          case frontend::resource_command::type::k_buffer:
            
            break;
          case frontend::resource_command::type::k_target:
            
            break;
          case frontend::resource_command::type::k_program:
            
            break;
          case frontend::resource_command::type::k_texture1D:
            
            break;
          case frontend::resource_command::type::k_texture2D:
            
            break;
          case frontend::resource_command::type::k_texture3D:
            
            break;
          case frontend::resource_command::type::k_textureCM:
            
            break;
        }
        break;
      }
      case frontend::command_type::k_draw: {
        const auto draw {reinterpret_cast<const frontend::draw_command*>(header + 1)};
        
        break;
      }
      case frontend::command_type::k_clear: {
        const auto clear {reinterpret_cast<const frontend::clear_command*>(header + 1)};
        
        break;
      }
      case frontend::command_type::k_blit: {
        
        const auto blit {reinterpret_cast<const frontend::blit_command*>(header + 1)};
        break;
      }
      case frontend::command_type::k_profile: {
        const auto profile {reinterpret_cast<const frontend::profile_command*>(header + 1)};
        
        break;
      }
    }
    
  });
  
}

void vk::process(rx_byte* _command) {
  
}

void vk::swap() {
  
}

allocation_info vk::query_allocation_info() const {
  allocation_info info {};
  info.buffer_size = sizeof(detail_vk::buffer);
  info.program_size = sizeof(detail_vk::program);
  info.target_size = sizeof(detail_vk::target);
  info.texture1D_size = sizeof(detail_vk::texture);
  info.texture2D_size = sizeof(detail_vk::texture);
  info.texture3D_size = sizeof(detail_vk::texture);
  info.textureCM_size = sizeof(detail_vk::texture);
  return info;
}

device_info vk::query_device_info() const {
  device_info info {};
  info.renderer = "";
  info.vendor = "";
  info.version = "";
  return info;
}

detail_vk::buffer::buffer(detail_vk::context& ctx_, frontend::buffer* buffer) {}

void detail_vk::buffer::destroy(detail_vk::context& ctx_, frontend::buffer* buffer) {
  
  if(handle != VK_NULL_HANDLE) vkDestroyBuffer(ctx_.device, handle, nullptr);
  
}


detail_vk::target::target(detail_vk::context& ctx_, frontend::target* target) {}
void detail_vk::target::destroy(detail_vk::context& ctx_, frontend::target* target) {}


detail_vk::program::program(detail_vk::context& ctx_, frontend::program* program) {}
void detail_vk::program::destroy(detail_vk::context& ctx_, frontend::program* program) {}



// textures
detail_vk::texture::texture(detail_vk::context& ctx_, frontend::texture* texture) {}

void detail_vk::texture::construct_base(detail_vk::context& ctx_, frontend::texture* texture, VkImageCreateInfo& info) {
  
  info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  info.arrayLayers = 1;
  info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  info.queueFamilyIndexCount = 1;
  info.pQueueFamilyIndices = &ctx_.graphics_index;
  info.samples = VK_SAMPLE_COUNT_1_BIT;
  info.tiling = VK_IMAGE_TILING_OPTIMAL;
  
  
  info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
  if(texture->kind() == frontend::texture::type::k_attachment) {
    if(texture->is_color_format()) info.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if(texture->is_depth_format() || texture->is_stencil_format() || texture->is_depth_stencil_format()) info.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  }
  
  switch(texture->format()) {
    case frontend::texture::data_format::k_bgr_u8:
      info.format = VK_FORMAT_B8G8R8_UINT;
      break;
    case frontend::texture::data_format::k_bgra_f16:
      info.format = VK_FORMAT_B16G16R16G16_422_UNORM;
      break;
    case frontend::texture::data_format::k_bgra_u8:
      info.format = VK_FORMAT_B8G8R8A8_UINT;
      break;
    case frontend::texture::data_format::k_d16:
      info.format = VK_FORMAT_D16_UNORM;
      break;
    case frontend::texture::data_format::k_d24:
      info.format = VK_FORMAT_D24_UNORM_S8_UINT;
      break;
    case frontend::texture::data_format::k_d24_s8:
      info.format = VK_FORMAT_D24_UNORM_S8_UINT;
      break;
    case frontend::texture::data_format::k_d32:
      info.format = VK_FORMAT_D32_SFLOAT;
      break;
    case frontend::texture::data_format::k_d32f:
      info.format = VK_FORMAT_D32_SFLOAT;
      break;
    case frontend::texture::data_format::k_d32f_s8:
      info.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
      break;
    case frontend::texture::data_format::k_dxt1:
      info.format = VK_FORMAT_UNDEFINED;
      break;
    case frontend::texture::data_format::k_dxt5:
      info.format = VK_FORMAT_UNDEFINED;
      break;
    case frontend::texture::data_format::k_r_u8:
      info.format = VK_FORMAT_R8_UINT;
      break;
    case frontend::texture::data_format::k_rgb_u8:
      info.format = VK_FORMAT_R8G8B8_UINT;
      break;
    case frontend::texture::data_format::k_rgba_f16:
      info.format = VK_FORMAT_R16G16B16A16_SFLOAT;
      break;
    case frontend::texture::data_format::k_rgba_u8:
      info.format = VK_FORMAT_R8G8B8A8_UINT;
      break;
    case frontend::texture::data_format::k_s8:
      info.format = VK_FORMAT_S8_UINT;
      break;
  }
  
  format = info.format;
  
}

void detail_vk::texture::construct_base_view(detail_vk::context& ctx_, frontend::texture* texture, VkImageViewCreateInfo& info) {
  
  info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  info.format = format;
  info.image = handle;
  info.subresourceRange.aspectMask = 0;
  if(texture->is_color_format()) info.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_COLOR_BIT;
  if(texture->is_depth_format()) info.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
  if(texture->is_stencil_format()) info.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
  info.subresourceRange.baseArrayLayer = 0;
  info.subresourceRange.baseMipLevel = 0;
  info.subresourceRange.layerCount = 1;
  info.subresourceRange.levelCount = 1;
  
}

void detail_vk::texture::destroy(detail_vk::context& ctx_, frontend::texture* texture) {
  
  if(view != VK_NULL_HANDLE) vkDestroyImageView(ctx_.device, view, nullptr);
  
  if(handle != VK_NULL_HANDLE) vkDestroyImage(ctx_.device, handle, nullptr);
  
}

detail_vk::buffer_builder::buffer_builder(detail_vk::context& ctx_) {
  
}

void detail_vk::buffer_builder::construct(detail_vk::context& ctx_, frontend::buffer* buffer) {
  
  auto buf = reinterpret_cast<detail_vk::buffer*>(buffer + 1);
  
  if(buffer->size() == 0) return;

  VkBufferCreateInfo info {};
  info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  info.queueFamilyIndexCount = 1;
  info.pQueueFamilyIndices = &ctx_.graphics_index;
  info.size = buffer->size();
  info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
  
  check_result(vkCreateBuffer(ctx_.device, &info, nullptr, &buf->handle));
  
  VkMemoryRequirements req;
  vkGetBufferMemoryRequirements(ctx_.device, buf->handle, &req);
  
  current_buffer_size = (current_buffer_size/req.alignment + 1)*req.alignment + req.size;
  buffer_type_bits &= req.memoryTypeBits;
  
}

void detail_vk::buffer_builder::interpass(detail_vk::context& ctx_) {
  
  if(current_buffer_size == 0) return;
  
  RX_ASSERT(buffer_type_bits > 0, "buffers cannot be in the same memory");
  
  {
    // buffer memory
    VkMemoryAllocateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    info.allocationSize = current_buffer_size;
    info.memoryTypeIndex = get_memory_type(ctx_, buffer_type_bits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | 
      ctx_.is_dedicated ? 0 : VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    check_result(vkAllocateMemory(ctx_.device, &info, nullptr, &buffer_memory));
    ctx_.image_allocations.push_back({buffer_memory});
    staging_memory = buffer_memory;
  }
  
  if(ctx_.is_dedicated) {
    // staging memory
    VkMemoryAllocateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    info.allocationSize = current_buffer_size;
    info.memoryTypeIndex = get_memory_type(ctx_, 0xffffffff, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    check_result(vkAllocateMemory(ctx_.device, &info, nullptr, &staging_memory));
    ctx_.staging_allocations.push_back({staging_memory});
  }
  
  void* ptr;
  vkMapMemory(ctx_.device, staging_memory, 0, current_buffer_size, 0, &ptr);
  buffer_staging_pointer = reinterpret_cast<rx_byte*>(ptr);
  
  current_buffer_size = 0;
  
}

void detail_vk::buffer_builder::construct2(detail_vk::context& ctx_, frontend::buffer* buffer) {
  
  auto buf = reinterpret_cast<detail_vk::buffer*>(buffer + 1);
  
  if(buffer->size() == 0) return;
  
  VkMemoryRequirements req;
  vkGetBufferMemoryRequirements(ctx_.device, buf->handle, &req);
  
  vkBindBufferMemory(ctx_.device, buf->handle, buffer_memory, current_buffer_size);
  
  memcpy(buffer_staging_pointer + current_buffer_size, buffer->vertices().data(), buffer->vertices().size());
  memcpy(buffer_staging_pointer + current_buffer_size + buffer->vertices().size(), buffer->elements().data(), buffer->elements().size());
  
  current_buffer_size = (current_buffer_size/req.alignment + 1)*req.alignment + req.size;
  
  if(ctx_.is_dedicated) {
    // TODO : copy
  }
  
}


detail_vk::texture_builder::texture_builder(detail_vk::context& ctx_) {
  
}

void detail_vk::texture_builder::construct(detail_vk::context& ctx_, const frontend::resource_command* resource) {
  
  detail_vk::texture* tex = nullptr;
  frontend::texture* texture = nullptr;
  switch(resource->kind) {
    case frontend::resource_command::type::k_texture1D:
      tex = reinterpret_cast<detail_vk::texture*>(resource->as_texture1D + 1);
      texture = resource->as_texture1D;
      break;
    case frontend::resource_command::type::k_texture2D:
      tex = reinterpret_cast<detail_vk::texture*>(resource->as_texture2D + 1);
      texture = resource->as_texture2D;
      break;
    case frontend::resource_command::type::k_texture3D:
      tex = reinterpret_cast<detail_vk::texture*>(resource->as_texture3D + 1);
      texture = resource->as_texture3D;
      break;
    case frontend::resource_command::type::k_textureCM:
      tex = reinterpret_cast<detail_vk::texture*>(resource->as_textureCM + 1);
      texture = resource->as_textureCM;
      break;
    default:
      break;
  }

  VkImageCreateInfo info {};
  tex->construct_base(ctx_, texture, info);

  switch(resource->kind) {
    case frontend::resource_command::type::k_texture1D: {
      info.mipLevels = 1;//texture->levels();
      info.extent.width = resource->as_texture1D->dimensions();
      info.extent.height = 1;
      info.extent.depth = 1;
      info.imageType = VK_IMAGE_TYPE_1D;
      break;
    }
    case frontend::resource_command::type::k_texture2D: {
      info.mipLevels = 1;//texture->levels();
      auto& dim = resource->as_texture2D->dimensions();
      info.extent.width = dim.x;
      info.extent.height = dim.y;
      info.extent.depth = 1;
      info.imageType = VK_IMAGE_TYPE_2D;
      break;
    }
    case frontend::resource_command::type::k_texture3D: {
      info.mipLevels = 1;//texture->levels();
      auto& dim = resource->as_texture3D->dimensions();
      info.extent.width = dim.x;
      info.extent.height = dim.y;
      info.extent.depth = dim.z;
      info.imageType = VK_IMAGE_TYPE_3D;
      break;
    }
    case frontend::resource_command::type::k_textureCM: {
      info.mipLevels = 1;//texture->levels();
      info.arrayLayers = 6;
      auto& dim = resource->as_textureCM->dimensions();
      info.extent.width = dim.x;
      info.extent.height = dim.y;
      info.extent.depth = 1;
      info.imageType = VK_IMAGE_TYPE_2D;
      info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
      break;
    }
    default:
      break;
  }


  check_result(vkCreateImage(ctx_.device, &info, nullptr, &tex->handle));

  VkMemoryRequirements req;
  vkGetImageMemoryRequirements(ctx_.device, tex->handle, &req);

  current_image_size = math::ceil((rx_f32) (current_image_size/req.alignment))*req.alignment + req.size;
  current_image_staging_size += texture->data().size() * sizeof(rx_byte);
  image_type_bits &= req.memoryTypeBits;
  
}

void detail_vk::texture_builder::interpass(detail_vk::context& ctx_) {
  
  if(current_image_size == 0) return;
  
  RX_ASSERT(image_type_bits > 0, "images cannot be in the same memory");
  
  // image memory
  {
    
    VkMemoryAllocateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    info.allocationSize = current_image_size;
    info.memoryTypeIndex = get_memory_type(ctx_, image_type_bits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | 
      ctx_.is_dedicated ? 0 : VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    check_result(vkAllocateMemory(ctx_.device, &info, nullptr, &image_memory));
    ctx_.image_allocations.push_back({image_memory});
    
  }
  
  // staging memory
  {
    
    VkMemoryAllocateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    info.allocationSize = current_image_staging_size;
    info.memoryTypeIndex = get_memory_type(ctx_, 0xffffffff, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    check_result(vkAllocateMemory(ctx_.device, &info, nullptr, &staging_memory));
    ctx_.staging_allocations.push_back({staging_memory});
    
    void* ptr;
    vkMapMemory(ctx_.device, staging_memory, 0, current_image_staging_size, 0, &ptr);
    image_staging_pointer = reinterpret_cast<rx_byte*>(ptr);
    
  }
  
  current_image_size = 0;
  current_image_staging_size = 0;
  
}

void detail_vk::texture_builder::construct2(detail_vk::context& ctx_, const frontend::resource_command* resource) {
  
  frontend::texture* texture = nullptr;
  detail_vk::texture* tex = nullptr;
  
  switch(resource->kind) {
    case frontend::resource_command::type::k_texture1D:
      texture = resource->as_texture1D;
      tex = reinterpret_cast<detail_vk::texture*>(resource->as_texture1D + 1);
      break;
    case frontend::resource_command::type::k_texture2D:
      texture = resource->as_texture2D;
      tex = reinterpret_cast<detail_vk::texture*>(resource->as_texture2D + 1);
      break;
    case frontend::resource_command::type::k_texture3D:
      texture = resource->as_texture3D;
      tex = reinterpret_cast<detail_vk::texture*>(resource->as_texture3D + 1);
      break;
    case frontend::resource_command::type::k_textureCM:
      texture = resource->as_textureCM;
      tex = reinterpret_cast<detail_vk::texture*>(resource->as_textureCM + 1);
      break;
    default:
      break;
  }
  
  VkMemoryRequirements req;
  vkGetImageMemoryRequirements(ctx_.device, tex->handle, &req);
  
  current_image_size = math::ceil((rx_f32) (current_image_size/req.alignment))*req.alignment;
  
  vkBindImageMemory(ctx_.device, tex->handle, image_memory, current_image_size);
  
  current_image_size += req.size;
  
  memcpy(image_staging_pointer + current_image_staging_size, texture->data().data(), texture->data().size());
  
  current_image_staging_size += texture->data().size() * sizeof(rx_byte);
  
  // TODO : copy
  
  
  
}



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
