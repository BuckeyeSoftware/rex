#include "data_builder.h"

#include "helper.h"

#include "rx/core/math/ceil.h"

namespace rx::render::backend {

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

detail_vk::buffer_builder::buffer_builder(detail_vk::context& ctx_) {}

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
  
  current_buffer_size = math::ceil((rx_f32) (current_buffer_size/req.alignment))*req.alignment;
  
  vkBindBufferMemory(ctx_.device, buf->handle, buffer_memory, current_buffer_size);
  
  memcpy(buffer_staging_pointer + current_buffer_size, buffer->vertices().data(), buffer->vertices().size());
  memcpy(buffer_staging_pointer + current_buffer_size + buffer->vertices().size(), buffer->elements().data(), buffer->elements().size());
  
  current_buffer_size += req.size;
  
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
  if(current_image_staging_size > 0) {
    
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
  
  if(texture->data().size() > 0) {
    
    memcpy(image_staging_pointer + current_image_staging_size, texture->data().data(), texture->data().size());
    
    current_image_staging_size += texture->data().size() * sizeof(rx_byte);
    
    // TODO : copy
    
  }
  
}

}
