#include "data_builder.h"

#include "context.h"

#include "helper.h"

#include "rx/core/math/ceil.h"
#include "rx/core/algorithm/max.h"

namespace rx::render::backend {

  
  
// buffer

detail_vk::buffer::buffer() {}

void detail_vk::buffer::destroy(detail_vk::context& ctx_, frontend::buffer* buffer) {
  
  if(handle != VK_NULL_HANDLE) vkDestroyBuffer(ctx_.device, handle, nullptr);
  
}


// buffer builder

detail_vk::buffer_builder::buffer_builder(detail_vk::context& ctx_) : buffer_infos(ctx_.allocator) {}

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
  
#if defined(RX_DEBUG)
  buf->name = ctx_.current_command->tag.description;
#endif
  SET_NAME(ctx_, VK_OBJECT_TYPE_BUFFER, buf->handle, buf->name);
  
  buf->last_use = use_queue::use_info(VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, ctx_.graphics_index, false, false);
  
  VkMemoryRequirements req;
  vkGetBufferMemoryRequirements(ctx_.device, buf->handle, &req);
  
  current_buffer_size = (current_buffer_size/req.alignment + 1)*req.alignment + req.size;
  buffer_type_bits &= req.memoryTypeBits;
  
}







void detail_vk::buffer_builder::build(detail_vk::context& ctx_) {
  
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
  
  buffer_infos.each_fwd([&](buffer_info& info) {
    construct2(ctx_, info);
  });
  
}







void detail_vk::buffer_builder::construct2(detail_vk::context& ctx_, buffer_info& info) {
  
  auto buffer = info.resource;
  
  auto buf = reinterpret_cast<const detail_vk::buffer*>(buffer + 1);
  
  if(buffer->size() == 0) return;
  
  VkMemoryRequirements req;
  vkGetBufferMemoryRequirements(ctx_.device, buf->handle, &req);
  
  current_buffer_size = math::ceil((rx_f32) (current_buffer_size)/req.alignment)*req.alignment;
  
  vkBindBufferMemory(ctx_.device, buf->handle, buffer_memory, current_buffer_size);
  
  memcpy(buffer_staging_pointer + current_buffer_size, buffer->vertices().data(), buffer->vertices().size());
  memcpy(buffer_staging_pointer + current_buffer_size + buffer->vertices().size(), buffer->elements().data(), buffer->elements().size());
  
  if (ctx_.is_dedicated) {
    
    VkCommandBuffer command = ctx_.transfer.get(ctx_);
    
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

detail_vk::texture::texture() : last_use(VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, VK_QUEUE_FAMILY_IGNORED, false, false) {}

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
    case frontend::texture::data_format::k_s8:
      info.format = VK_FORMAT_S8_UINT;
      break;
      
      
      
    case frontend::texture::data_format::k_r_u8:
      info.format = VK_FORMAT_R8_UNORM;
      format_size = 1;
      break;
    case frontend::texture::data_format::k_rgb_u8:
      info.format = VK_FORMAT_R8G8B8_UNORM;
      format_size = 3;
      break;
    case frontend::texture::data_format::k_rgba_u8:
      info.format = VK_FORMAT_R8G8B8A8_UNORM;
      format_size = 4;
      break;
    case frontend::texture::data_format::k_srgb_u8:
      info.format = VK_FORMAT_R8G8B8_SRGB;
      format_size = 3;
      break;
    case frontend::texture::data_format::k_srgba_u8:
      info.format = VK_FORMAT_R8G8B8A8_SRGB;
      format_size = 4;
      break;
    case frontend::texture::data_format::k_rgba_f16:
      info.format = VK_FORMAT_R16G16B16A16_SFLOAT;
      format_size = 8;
      break;
      
    case frontend::texture::data_format::k_bgr_u8:
      info.format = VK_FORMAT_B8G8R8_UNORM;
      format_size = 3;
      break;
    case frontend::texture::data_format::k_bgra_u8:
      info.format = VK_FORMAT_B8G8R8A8_UNORM;
      format_size = 4;
      break;
      
    case frontend::texture::data_format::k_dxt5:
    case frontend::texture::data_format::k_dxt1:
    case frontend::texture::data_format::k_bgra_f16:
      info.format = VK_FORMAT_UNDEFINED;
      vk_log(log::level::k_error, "Format not supported");
      break;
  }
  
  format = info.format;
  
  VkFormatProperties props;
  vkGetPhysicalDeviceFormatProperties(ctx_.physical, format, &props);
  
  if(props.optimalTilingFeatures == 0) {
    
    switch(texture->format()) {
      case frontend::texture::data_format::k_d16:
      case frontend::texture::data_format::k_d24:
      case frontend::texture::data_format::k_d24_s8:
        format = VK_FORMAT_D32_SFLOAT_S8_UINT;
        break;
      case frontend::texture::data_format::k_d32:
      case frontend::texture::data_format::k_d32f:
        format = VK_FORMAT_X8_D24_UNORM_PACK32;
        break;
      case frontend::texture::data_format::k_d32f_s8:
        format = VK_FORMAT_D24_UNORM_S8_UINT;
        break;
      case frontend::texture::data_format::k_s8:
        format = VK_FORMAT_S8_UINT;
        break;
        
      case frontend::texture::data_format::k_r_u8:
        format = VK_FORMAT_R8_UNORM;
        format_size = 1;
        break;
      case frontend::texture::data_format::k_rgba_f16:
        format = VK_FORMAT_R16G16B16A16_SFLOAT;
        format_size = 8;
        break;
        
      case frontend::texture::data_format::k_rgb_u8:
      case frontend::texture::data_format::k_rgba_u8:
        format = VK_FORMAT_R8G8B8A8_UNORM;
        format_size = 4;
        break;
        
      case frontend::texture::data_format::k_srgb_u8:
      case frontend::texture::data_format::k_srgba_u8:
        format = VK_FORMAT_R8G8B8A8_SRGB;
        format_size = 4;
        break;
        
      case frontend::texture::data_format::k_bgr_u8:
      case frontend::texture::data_format::k_bgra_u8:
        format = VK_FORMAT_B8G8R8A8_UNORM;
        format_size = 4;
        break;
        
      case frontend::texture::data_format::k_dxt5:
      case frontend::texture::data_format::k_dxt1:
      case frontend::texture::data_format::k_bgra_f16:
        format = VK_FORMAT_UNDEFINED;
    }
    
    vk_log(log::level::k_info, "format not natively supported : %d, changed to %d", info.format, format);
    info.format = format;
    
    vkGetPhysicalDeviceFormatProperties(ctx_.physical, format, &props);
    
    if(props.optimalTilingFeatures == 0) vk_log(log::level::k_error, "format still not supported : %d !", format);
    
  }
  
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




void detail_vk::texture::destroy(detail_vk::context& ctx_, frontend::texture* texture) {
  
  if (handle != VK_NULL_HANDLE) vkDestroyImage(ctx_.device, handle, nullptr);
  
}








// ------------------------------------
// texture builder

detail_vk::texture_builder::texture_builder(detail_vk::context& ctx_) : texture_infos(ctx_.allocator) {}

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
      info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
      break;
    }
    default:
      break;
  }
  
  tex->layers = info.arrayLayers;
  
  info.extent = tex->extent;

  check_result(vkCreateImage(ctx_.device, &info, nullptr, &tex->handle));
  
#if defined(RX_DEBUG)
  tex->name = ctx_.current_command->tag.description;
  SET_NAME(ctx_, VK_OBJECT_TYPE_IMAGE, tex->handle, tex->name);
#endif
  
  tex->last_use = use_queue::use_info(VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, ctx_.graphics_index, false, false);

  VkMemoryRequirements req;
  vkGetImageMemoryRequirements(ctx_.device, tex->handle, &req);
  
  
  image_type_bits &= req.memoryTypeBits;

  
  if(current_image_size % req.alignment != 0) {
    current_image_size = (current_image_size / req.alignment + 1) * req.alignment;
  }
  VkDeviceSize bind_offset = current_image_size;
  current_image_size += req.size;
  
  use_queue::use_info* use_info = nullptr;
  VkDeviceSize staging_offset = 0;
  
  if(texture->data().size() > 0) {
    
    staging_offset = current_image_staging_size;
    
    rx_size texture_size = texture->data().size() * sizeof(rx_byte) * tex->format_size / texture->byte_size_of_format(texture->format());
    rx_size alignment = 4;
    if(current_image_staging_size % alignment != 0) {
      current_image_staging_size = (current_image_staging_size / alignment + 1) * alignment;
    }
    
    use_info = tex->add_use(ctx_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, ctx_.graphics_index, true);
    
    current_image_staging_size += texture_size;
    
  }
  
  texture_infos.push_back({resource, use_info, staging_offset, bind_offset});
  
}









void detail_vk::texture_builder::build(detail_vk::context& ctx_) {
  
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
  
  texture_infos.each_fwd([&](texture_info& info) {
    construct2(ctx_, info);
  });
  
  vkUnmapMemory(ctx_.device, staging_memory);
  
}










void detail_vk::texture_builder::construct2(detail_vk::context& ctx_, texture_info& info) {
  
  auto resource = info.resource;
  
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
      return;
  }
  
  VkMemoryRequirements req;
  vkGetImageMemoryRequirements(ctx_.device, tex->handle, &req);
  
  // align current offset
  if(current_image_size % req.alignment != 0) {
    current_image_size = (current_image_size / req.alignment + 1) * req.alignment;
  }
  
  vk_log(log::level::k_verbose, "current_size %li : %li", current_image_size, info.bind_offset);
  
  vkBindImageMemory(ctx_.device, tex->handle, image_memory, current_image_size);
  
  current_image_size += req.size;
  
  if(texture->data().size() > 0) {
    
    rx_size alignment = 4;
    
    if(current_image_staging_size % alignment != 0) {
      current_image_staging_size = (current_image_staging_size / alignment + 1) * alignment;
    }
    
    rx_size byte_size = texture->byte_size_of_format(texture->format());
    rx_size texture_size = texture->data().size() * sizeof(rx_byte) / byte_size * tex->format_size;
    
    if(texture->byte_size_of_format(texture->format()) < tex->format_size) {
      rx_size count = {texture->data().size() * sizeof(rx_byte) / byte_size};
      for(rx_size i {0}; i<count; i++) {
        memcpy(image_staging_pointer + current_image_staging_size + i * tex->format_size, texture->data().data() + i * byte_size, byte_size);
      }
    } else {
      memcpy(image_staging_pointer + current_image_staging_size, texture->data().data(), texture->data().size() * sizeof(rx_byte));
    }
    
    
    VkCommandBuffer command = ctx_.transfer.get(ctx_);
    
    tex->sync(ctx_, texture, info.use_info, command);
    
    
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
      
      rx_size offset = 0;
      
      switch(resource->kind) {
        case frontend::resource_command::type::k_texture1D: {
          auto texture = resource->as_texture1D;
          auto level_info = texture->info_for_level(level);
          offset = level_info.offset;
          copy.imageExtent.width = level_info.dimensions;
          break;
        }
        case frontend::resource_command::type::k_texture2D: {
          auto texture = resource->as_texture2D;
          auto level_info = texture->info_for_level(level);
          offset = level_info.offset;
          auto dim  = level_info.dimensions;
          copy.imageExtent.width = dim.x;
          copy.imageExtent.height = dim.y;
          break;
        }
        case frontend::resource_command::type::k_texture3D: {
          auto texture = resource->as_texture3D;
          auto level_info = texture->info_for_level(level);
          offset = level_info.offset;
          auto dim  = level_info.dimensions;
          copy.imageExtent.width = dim.x;
          copy.imageExtent.height = dim.y;
          copy.imageExtent.depth = dim.z;
          break;
        }
        case frontend::resource_command::type::k_textureCM: {
          auto texture = resource->as_textureCM;
          auto level_info = texture->info_for_level(level);
          offset = level_info.offset;
          auto dim  = level_info.dimensions;
          copy.imageExtent.width = dim.x;
          copy.imageExtent.height = dim.y;
          copy.imageSubresource.layerCount = 6;
          break;
        }
        default:
          break;
      }
      
      copy.bufferOffset = current_image_staging_size + offset  * sizeof(rx_byte) / byte_size * tex->format_size;
      
    }
    
    vkCmdCopyBufferToImage(command, staging_buffer, tex->handle, tex->current_layout, texture->levels(), &copies[0]);
    
    current_image_staging_size = current_image_staging_size + texture_size;
    
  }
  
  
}


}
