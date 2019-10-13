#include "data_builder.h"

#include "context.h"

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
      info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
      break;
    }
    default:
      break;
  }
  
  info.extent = tex->extent;


  check_result(vkCreateImage(ctx_.device, &info, nullptr, &tex->handle));

  VkMemoryRequirements req;
  vkGetImageMemoryRequirements(ctx_.device, tex->handle, &req);

  current_image_size = math::ceil((rx_f32) (current_image_size)/req.alignment)*req.alignment + req.size;
  current_image_staging_size = math::ceil((rx_f32) (current_image_staging_size + texture->data().size() * sizeof(rx_byte))/4)*4;
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
    
    {
      VkBufferCreateInfo info {};
      info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      info.size = current_image_staging_size;
      info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
      info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      info.queueFamilyIndexCount = 1;
      info.pQueueFamilyIndices = &ctx_.graphics_index;
      
      check_result(vkCreateBuffer(ctx_.device, &info, nullptr, &staging_buffer));
      
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
  
  current_image_size = math::ceil((rx_f32) (current_image_size)/req.alignment)*req.alignment;
  
  vkBindImageMemory(ctx_.device, tex->handle, image_memory, current_image_size);
  
  current_image_size += req.size;
  
  if(texture->data().size() > 0) {
    
    memcpy(image_staging_pointer + current_image_staging_size, texture->data().data(), texture->data().size() * sizeof(rx_byte));
    
    VkCommandBuffer command = ctx_.transfer.get();
    
    VkImageMemoryBarrier barrier {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = tex->handle;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = texture->levels();
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.srcQueueFamilyIndex = ctx_.graphics_index;
    barrier.dstQueueFamilyIndex = ctx_.graphics_index;
    
    if(resource->kind == frontend::resource_command::type::k_textureCM) {
        barrier.subresourceRange.layerCount = 6;
    }
    
    vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT,
                         0, nullptr, 0, nullptr, 1, &barrier);
    
    
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
          vk_log(log::level::k_verbose, "%u, %d, %d", current_image_staging_size, level_info.offset, texture->data().size());
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
    
    vkCmdCopyBufferToImage(command, staging_buffer, tex->handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, texture->levels(), &copies[0]);
    
    
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT,
                         0, nullptr, 0, nullptr, 1, &barrier);
    
    current_image_staging_size = math::ceil( (rx_f32) (current_image_staging_size + texture->data().size() * sizeof(rx_byte)) / 4) * 4;
    
  }
  
  
}










void detail_vk::Transfer::init(detail_vk::context& ctx_) {
  
  {
    VkCommandPoolCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    info.queueFamilyIndex = ctx_.graphics_index;
    
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

void detail_vk::Transfer::start(detail_vk::context& ctx_) {
  
  if(!written) return;
  written = false;
  
  VkCommandBufferBeginInfo info {};
  info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  //info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  check_result(vkBeginCommandBuffer(commands[index], &info));
  
}

void detail_vk::Transfer::end(detail_vk::context& ctx_) {
  
  if (!written) return;
  
  VkCommandBuffer command = commands[index];
  check_result(vkEndCommandBuffer(command));
  
  VkSubmitInfo info {};
  info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  info.commandBufferCount = 1;
  info.pCommandBuffers = &command;
  check_result(vkQueueSubmit(ctx_.graphics, 1, &info, fences[index]));
  
  check_result(vkWaitForFences(ctx_.device, 1, &fences[index], VK_TRUE, 10000000000000L));
  
  check_result(vkResetFences(ctx_.device, 1, &fences[index]));
  
}

void detail_vk::Transfer::destroy(detail_vk::context& ctx_) {
  
  for (rx_size i {0}; i<fences.size(); i++) {
    vkDestroyFence(ctx_.device, fences[i], nullptr);
  }
  
  vkDestroyCommandPool(ctx_.device, pool, nullptr);
  
}

VkCommandBuffer detail_vk::Transfer::get() {
  
  written = true;
  return commands[index];
  
}

}
