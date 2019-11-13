#include "data_builder.h"

#include "context.h"

#include "helper.h"

#include "rx/core/math/ceil.h"
#include "rx/core/algorithm/max.h"

namespace rx::render::backend {

  
  
// buffer

void detail_vk::buffer::destroy(detail_vk::context& ctx_, frontend::buffer* buffer) {
  
  if(handle != VK_NULL_HANDLE) vkDestroyBuffer(ctx_.device, handle, nullptr);
  
}


// buffer builder

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
    
    {
      VkBufferCreateInfo info {};
      info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      info.size = current_buffer_size;
      info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
      info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      info.queueFamilyIndexCount = 1;
      info.pQueueFamilyIndices = &ctx_.graphics_index;
      
      check_result(vkCreateBuffer(ctx_.device, &info, nullptr, &staging_buffer));
      
      VkMemoryRequirements req;
      vkGetBufferMemoryRequirements(ctx_.device, staging_buffer, &req);
      ctx_.staging_allocation_buffers.push_back(staging_buffer);
      
      current_buffer_size = req.size;
      
    }
    
    { // staging memory
      
      VkMemoryAllocateInfo info {};
      info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      info.allocationSize = current_buffer_size;
      info.memoryTypeIndex = get_memory_type(ctx_, 0xffffffff, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
      check_result(vkAllocateMemory(ctx_.device, &info, nullptr, &staging_memory));
      ctx_.staging_allocations.push_back({staging_memory});
      
    }
    
    check_result(vkBindBufferMemory(ctx_.device, staging_buffer, staging_memory, 0));
    
  }
  
  void* ptr;
  check_result(vkMapMemory(ctx_.device, staging_memory, 0, current_buffer_size, 0, &ptr));
  buffer_staging_pointer = reinterpret_cast<rx_byte*>(ptr);
  
  current_buffer_size = 0;
  
}







void detail_vk::buffer_builder::construct2(detail_vk::context& ctx_, frontend::buffer* buffer) {
  
  auto buf = reinterpret_cast<detail_vk::buffer*>(buffer + 1);
  
  if(buffer->size() == 0) return;
  
  VkMemoryRequirements req;
  vkGetBufferMemoryRequirements(ctx_.device, buf->handle, &req);
  
  current_buffer_size = math::ceil((rx_f32) (current_buffer_size)/req.alignment)*req.alignment;
  
  vkBindBufferMemory(ctx_.device, buf->handle, buffer_memory, current_buffer_size);
  
  memcpy(buffer_staging_pointer + current_buffer_size, buffer->vertices().data(), buffer->vertices().size());
  memcpy(buffer_staging_pointer + current_buffer_size + buffer->vertices().size(), buffer->elements().data(), buffer->elements().size());
  
  if (ctx_.is_dedicated) {
    
    VkCommandBuffer command = ctx_.transfer.get();
    
    VkBufferCopy copy {};
    copy.srcOffset = current_buffer_size;
    copy.dstOffset = 0;
    copy.size = buffer->size();
    vkCmdCopyBuffer(command, staging_buffer, buf->handle, 1, &copy);
    
  }
  
  current_buffer_size += req.size;
  
}











// --------------------------------
// textures

void detail_vk::texture::construct_base(detail_vk::context& ctx_, frontend::texture* texture, VkImageCreateInfo& info) {
  
  info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  info.arrayLayers = 1;
  info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  info.queueFamilyIndexCount = 1;
  info.pQueueFamilyIndices = &ctx_.graphics_index;
  info.samples = VK_SAMPLE_COUNT_1_BIT;
  info.tiling = VK_IMAGE_TILING_OPTIMAL;
  
  
  info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  if(texture->kind() == frontend::texture::type::k_attachment) {
    if(texture->is_color_format()) info.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if(texture->is_depth_format() || texture->is_stencil_format() || texture->is_depth_stencil_format()) info.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  } else {
    info.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  }
  
  switch(texture->format()) {
    case frontend::texture::data_format::k_bgr_u8:
      info.format = VK_FORMAT_B8G8R8_UNORM;
      break;
    case frontend::texture::data_format::k_bgra_f16:
      info.format = VK_FORMAT_B16G16R16G16_422_UNORM;
      break;
    case frontend::texture::data_format::k_bgra_u8:
      info.format = VK_FORMAT_B8G8R8A8_UNORM;
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
      info.format = VK_FORMAT_R8G8B8_UNORM;
      break;
    case frontend::texture::data_format::k_rgba_f16:
      info.format = VK_FORMAT_R16G16B16A16_SFLOAT;
      break;
    case frontend::texture::data_format::k_rgba_u8:
      info.format = VK_FORMAT_R8G8B8A8_UNORM;
      break;
    case frontend::texture::data_format::k_s8:
      info.format = VK_FORMAT_S8_UINT;
      break;
  }
  
  format = info.format;
  
}


VkImageView detail_vk::texture::make_attachment(detail_vk::context& ctx_, frontend::texture* texture, uint32_t layer, uint32_t level) {
  
  VkImageView view;
  
  VkImageViewCreateInfo info {};
  info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  info.format = format;
  info.image = handle;
  if(texture->is_color_format()) info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  if(texture->is_depth_format()) info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  if(texture->is_stencil_format()) info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
  if(texture->is_depth_stencil_format()) info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_DEPTH_BIT;
  info.subresourceRange.baseArrayLayer = layer;
  info.subresourceRange.baseMipLevel = level;
  info.subresourceRange.layerCount = 1;
  info.subresourceRange.levelCount = 1;
  info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  
  check_result(vkCreateImageView(ctx_.device, &info, nullptr, &view));
  
  return view;
  
}


void detail_vk::texture::transfer_layout(context& ctx_, frontend::texture* texture, VkCommandBuffer command, texture_use_queue::texture_use_info use_info) {
  
  VkImageMemoryBarrier barrier {};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.image = handle;
  barrier.oldLayout = current_layout;
  barrier.newLayout = use_info.required_layout;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = layers;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = texture->levels();
  barrier.srcAccessMask = 0;
  barrier.dstAccessMask = use_info.use_access;
  barrier.srcQueueFamilyIndex = ctx_.graphics_index;
  barrier.dstQueueFamilyIndex = ctx_.graphics_index;
  
  vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, use_info.use_stage, VK_DEPENDENCY_BY_REGION_BIT,
                          0, nullptr, 0, nullptr, 1, &barrier);
  
}


void detail_vk::texture::destroy(detail_vk::context& ctx_, frontend::texture* texture) {
  
  if (handle != VK_NULL_HANDLE) vkDestroyImage(ctx_.device, handle, nullptr);
  
}








// ------------------------------------
// texture builder

detail_vk::texture_builder::texture_builder(detail_vk::context& ctx_) {
  
}

void detail_vk::texture_builder::construct(detail_vk::context& ctx_, const frontend::resource_command* resource) {
  
  detail_vk::texture* tex = nullptr;
  frontend::texture* texture = nullptr;
  switch (resource->kind) {
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
  
  tex->current_layout = VK_IMAGE_LAYOUT_UNDEFINED;

  VkImageCreateInfo info {};
  tex->construct_base(ctx_, texture, info);

  switch (resource->kind) {
    case frontend::resource_command::type::k_texture1D: {
      info.mipLevels = texture->levels();
      tex->extent.width = resource->as_texture1D->dimensions();
      tex->extent.height = 1;
      tex->extent.depth = 1;
      info.imageType = VK_IMAGE_TYPE_1D;
      break;
    }
    case frontend::resource_command::type::k_texture2D: {
      info.mipLevels = texture->levels();
      auto& dim = resource->as_texture2D->dimensions();
      tex->extent.width = dim.x;
      tex->extent.height = dim.y;
      tex->extent.depth = 1;
      info.imageType = VK_IMAGE_TYPE_2D;
      break;
    }
    case frontend::resource_command::type::k_texture3D: {
      info.mipLevels = texture->levels();
      auto& dim = resource->as_texture3D->dimensions();
      tex->extent.width = dim.x;
      tex->extent.height = dim.y;
      tex->extent.depth = dim.z;
      info.imageType = VK_IMAGE_TYPE_3D;
      break;
    }
    case frontend::resource_command::type::k_textureCM: {
      info.mipLevels = texture->levels();
      info.arrayLayers = 6;
      auto& dim = resource->as_textureCM->dimensions();
      tex->extent.width = dim.x;
      tex->extent.height = dim.y;
      tex->extent.depth = 1;
      info.imageType = VK_IMAGE_TYPE_2D;
      info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
      break;
    }
    default:
      break;
  }
  
  tex->layers = info.arrayLayers;
  
  info.extent = tex->extent;

  check_result(vkCreateImage(ctx_.device, &info, nullptr, &tex->handle));

  VkMemoryRequirements req;
  vkGetImageMemoryRequirements(ctx_.device, tex->handle, &req);

  current_image_size = math::ceil((rx_f32) (current_image_size + req.size)/req.alignment)*req.alignment;
  rx_size alignment = texture->byte_size_of_format(texture->format());
  if(alignment < 4) alignment *= 4;
  current_image_staging_size = math::ceil((rx_f32) (current_image_staging_size + texture->data().size() * sizeof(rx_byte))/alignment)*alignment;
  image_type_bits &= req.memoryTypeBits;
  
  tex->frame_uses.push(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, false, true, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT);
  
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
    
    {
      
      VkBufferCreateInfo info {};
      info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      info.size = current_image_staging_size;
      info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
      info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      info.queueFamilyIndexCount = 1;
      info.pQueueFamilyIndices = &ctx_.graphics_index;
      
      check_result(vkCreateBuffer(ctx_.device, &info, nullptr, &staging_buffer));
      ctx_.staging_allocation_buffers.push_back(staging_buffer);
      
      VkMemoryRequirements req;
      vkGetBufferMemoryRequirements(ctx_.device, staging_buffer, &req);
      
      current_image_staging_size = req.size;
      
    }
    
    VkMemoryAllocateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    info.allocationSize = current_image_staging_size;
    info.memoryTypeIndex = get_memory_type(ctx_, 0xffffffff, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    check_result(vkAllocateMemory(ctx_.device, &info, nullptr, &staging_memory));
    ctx_.staging_allocations.push_back({staging_memory});
    
    check_result(vkBindBufferMemory(ctx_.device, staging_buffer, staging_memory, 0));
    
    void* ptr;
    check_result(vkMapMemory(ctx_.device, staging_memory, 0, current_image_staging_size, 0, &ptr));
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
  
  vkBindImageMemory(ctx_.device, tex->handle, image_memory, current_image_size);
  
  current_image_size = math::ceil((rx_f32) (current_image_size + req.size)/req.alignment)*req.alignment;
  
  if(texture->data().size() > 0) {
    
    memcpy(image_staging_pointer + current_image_staging_size, texture->data().data(), texture->data().size() * sizeof(rx_byte));
    
    VkCommandBuffer command = ctx_.transfer.get();
    
    tex->transfer_layout(ctx_, texture, command, tex->frame_uses.pop());
    
    tex->current_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    
    
    VkBufferImageCopy copies [texture->levels()];
    
    for (rx_size level {0}; level < texture->levels(); level++) {
      
      VkBufferImageCopy& copy = copies[level];
      
      copy.bufferImageHeight = 0;
      copy.bufferRowLength = 0;
      copy.imageOffset.x = 0;
      copy.imageOffset.y = 0;
      copy.imageOffset.z = 0;
      copy.imageExtent.width = 1;
      copy.imageExtent.height = 1;
      copy.imageExtent.depth = 1;
      copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      copy.imageSubresource.baseArrayLayer = 0;
      copy.imageSubresource.layerCount = 1;
      
      copy.imageSubresource.mipLevel = level;
      
      switch(resource->kind) {
        case frontend::resource_command::type::k_texture1D: {
          auto texture = resource->as_texture1D;
          auto level_info = texture->info_for_level(level);
          copy.bufferOffset = current_image_staging_size + level_info.offset;
          copy.imageExtent.width = level_info.dimensions;
          break;
        }
        case frontend::resource_command::type::k_texture2D: {
          auto texture = resource->as_texture2D;
          auto level_info = texture->info_for_level(level);
          copy.bufferOffset = current_image_staging_size + level_info.offset;
          auto dim  = level_info.dimensions;
          copy.imageExtent.width = dim.x;
          copy.imageExtent.height = dim.y;
          break;
        }
        case frontend::resource_command::type::k_texture3D: {
          auto texture = resource->as_texture3D;
          auto level_info = texture->info_for_level(level);
          copy.bufferOffset = current_image_staging_size + level_info.offset;
          auto dim  = level_info.dimensions;
          copy.imageExtent.width = dim.x;
          copy.imageExtent.height = dim.y;
          copy.imageExtent.depth = dim.z;
          break;
        }
        case frontend::resource_command::type::k_textureCM: {
          auto texture = resource->as_textureCM;
          auto level_info = texture->info_for_level(level);
          copy.bufferOffset = current_image_staging_size + level_info.offset;
          auto dim  = level_info.dimensions;
          copy.imageExtent.width = dim.x;
          copy.imageExtent.height = dim.y;
          copy.imageSubresource.layerCount = 6;
          break;
        }
        default:
          break;
      }
      
    }
    
    vkCmdCopyBufferToImage(command, staging_buffer, tex->handle, tex->current_layout, texture->levels(), &copies[0]);
    
    rx_size alignment = texture->byte_size_of_format(texture->format());
    if(alignment < 4) alignment *= 4;
    current_image_staging_size = math::ceil( (rx_f32) (current_image_staging_size + texture->data().size() * sizeof(rx_byte)) / alignment) * alignment;
    
  }
  
  
}


}
