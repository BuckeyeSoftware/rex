#include "sync.h"

#include "data_builder.h"
#include "context.h"

namespace rx::render::backend {

void detail_vk::buffer::sync(context& ctx_, frontend::buffer* buffer, const use_queue::use_info* last_use, VkCommandBuffer command) {
  
  (void) ctx_;
  (void) buffer;
  (void) command;
  (void) last_use;
  
}


void detail_vk::texture::sync(context& ctx_, frontend::texture* texture, const use_queue::use_info* last_use, VkCommandBuffer command) {
  
  auto current_use = last_use->after;
  
  if(!last_use->sync_after && (last_use->layout != current_use->layout || last_use->write || last_use->queue != current_use->queue)) {
    
#if defined(RX_DEBUG)
    vk_log(log::level::k_verbose, "transferring image from %s to %s : %s", layout_to_string(last_use->layout), layout_to_string(current_use->layout), name);
#endif
    
    VkImageMemoryBarrier barrier {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = handle;
    barrier.oldLayout = last_use->layout;
    barrier.newLayout = current_use->layout;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = layers;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = texture->levels();
    barrier.srcAccessMask = last_use->access;
    barrier.dstAccessMask = current_use->access;
    barrier.srcQueueFamilyIndex = last_use->queue;
    barrier.dstQueueFamilyIndex = current_use->queue;
    
    vkCmdPipelineBarrier(command, last_use->stage, current_use->stage, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &barrier);
    
    current_layout = current_use->layout;
    
  }
  
}




detail_vk::use_queue::use_info* detail_vk::buffer::add_use(context& ctx_, VkPipelineStageFlags stage, VkAccessFlags access, uint32_t queue, bool write, bool sync_after) {
  
  return ctx_.sync->get_buffer_uses(ctx_, this).add_use(ctx_, VK_IMAGE_LAYOUT_UNDEFINED, stage, access, queue, write, sync_after);
  
}

detail_vk::use_queue::use_info* detail_vk::texture::add_use(context& ctx_, VkImageLayout layout, VkPipelineStageFlags stage, VkAccessFlags access, uint32_t queue, bool write, bool sync_after) {
  
  return ctx_.sync->get_texture_uses(ctx_, this).add_use(ctx_, layout, stage, access, queue, write, sync_after);
  
}





detail_vk::use_queue::use_queue(context& ctx_, use_info& first) {
  
  tail = ctx_.allocator->create<use_info>(first);
  head = tail;
  
}

detail_vk::use_queue::use_info* detail_vk::use_queue::add_use(context& ctx_, VkImageLayout layout, VkPipelineStageFlags stage, VkAccessFlags access, uint32_t queue, bool write, bool sync_after) {
  
  auto last_use = head;
  
  head = ctx_.allocator->create<use_info>(layout, stage, access, queue, write, sync_after);
  
  last_use->after = head;
  
  return last_use;
  
}

detail_vk::use_queue::use_info detail_vk::use_queue::clear(context& ctx_) {
  
  use_info hd = *head;
  
  while (tail != nullptr) {
    auto last = tail;
    tail = tail->after;
    ctx_.allocator->destroy<use_info>(last);
  }
  
  return hd;
  
}

detail_vk::resource_sync::resource_sync(context& ctx_) : buffer_queues(ctx_.allocator), texture_queues(ctx_.allocator) {}

detail_vk::use_queue& detail_vk::resource_sync::get_buffer_uses(context& ctx_, buffer* buffer) {
  auto q = buffer_queues.find(buffer);
  if(q == nullptr) {
    return *buffer_queues.insert(buffer, use_queue(ctx_, buffer->last_use));
  }
  return *q;
}

detail_vk::use_queue& detail_vk::resource_sync::get_texture_uses(context& ctx_, texture* texture) {
  auto q = texture_queues.find(texture);
  if(q == nullptr) {
    return *texture_queues.insert(texture, use_queue(ctx_, texture->last_use));
  }
  return *q;
}

void detail_vk::resource_sync::clear(context& ctx_) {
  
  this->buffer_queues.each_pair([&](buffer* buffer, use_queue& use_queue) {
    buffer->last_use = use_queue.clear(ctx_);
  });
  
  this->texture_queues.each_pair([&](texture* texture, use_queue& use_queue) {
    texture->last_use = use_queue.clear(ctx_);
  });
  
}

}
