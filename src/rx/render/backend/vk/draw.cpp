#include "draw.h"

#include "context.h"

#include "data_builder.h"

namespace rx::render::backend {
  
void detail_vk::frame_render::pre_clear(detail_vk::context& ctx_, const frontend::clear_command* clear) {
  
  
  
}

void detail_vk::frame_render::clear(detail_vk::context& ctx_, const frontend::clear_command* clear) {
  
  
  
}

void detail_vk::frame_render::pre_blit(detail_vk::context& ctx_, const frontend::blit_command* blit) {
  
  auto& src_texture = blit->src_target->attachments()[blit->src_attachment];
  auto src_tex = (src_texture.kind == frontend::target::attachment::type::k_textureCM ? reinterpret_cast<detail_vk::texture*>(src_texture.as_textureCM.texture + 1) : reinterpret_cast<detail_vk::texture*>(src_texture.as_texture2D.texture + 1));
  auto& dst_texture = blit->dst_target->attachments()[blit->dst_attachment];
  auto dst_tex = (dst_texture.kind == frontend::target::attachment::type::k_textureCM ? reinterpret_cast<detail_vk::texture*>(dst_texture.as_textureCM.texture + 1) : reinterpret_cast<detail_vk::texture*>(dst_texture.as_texture2D.texture + 1));
  
  src_tex->frame_uses.push(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, true, true, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT);
  dst_tex->frame_uses.push(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, true, true, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT);
  
}

void detail_vk::frame_render::blit(detail_vk::context& ctx_, const frontend::blit_command* blit) {
  
  auto& src_texture = blit->src_target->attachments()[blit->src_attachment];
  auto src_tex = (src_texture.kind == frontend::target::attachment::type::k_textureCM ? reinterpret_cast<detail_vk::texture*>(src_texture.as_textureCM.texture + 1) : reinterpret_cast<detail_vk::texture*>(src_texture.as_texture2D.texture + 1));
  uint32_t src_layer = (src_texture.kind == frontend::target::attachment::type::k_textureCM ? static_cast<uint8_t> (src_texture.as_textureCM.face) : 0);
  
  auto& dst_texture = blit->dst_target->attachments()[blit->dst_attachment];
  auto dst_tex = (dst_texture.kind == frontend::target::attachment::type::k_textureCM ? reinterpret_cast<detail_vk::texture*>(dst_texture.as_textureCM.texture + 1) : reinterpret_cast<detail_vk::texture*>(dst_texture.as_texture2D.texture + 1));
  uint32_t dst_layer = (src_texture.kind == frontend::target::attachment::type::k_textureCM ? static_cast<uint8_t> (src_texture.as_textureCM.face) : 0);
  
  VkCommandBuffer command = ctx_.graphics.get();
  
  
  {
    auto& use = src_tex->frame_uses.next();
    if(src_tex->current_layout != use.required_layout || (use.sync_before && src_tex->frame_uses.has_last() && src_tex->frame_uses.last().sync_after) ) {
      src_tex->transfer_layout(ctx_,
                              (src_texture.kind == frontend::target::attachment::type::k_textureCM ? reinterpret_cast<frontend::texture*>(src_texture.as_textureCM.texture) : reinterpret_cast<frontend::texture*>(src_texture.as_texture2D.texture)),
                              command, use);
    }
    src_tex->frame_uses.pop();
  }
  
  {
    auto& use = dst_tex->frame_uses.next();
    if(dst_tex->current_layout != use.required_layout || (use.sync_before && dst_tex->frame_uses.has_last() && dst_tex->frame_uses.last().sync_after) ) {
      dst_tex->transfer_layout(ctx_,
                              (dst_texture.kind == frontend::target::attachment::type::k_textureCM ? reinterpret_cast<frontend::texture*>(dst_texture.as_textureCM.texture) : reinterpret_cast<frontend::texture*>(dst_texture.as_texture2D.texture)),
                              command, use);
    }
    dst_tex->frame_uses.pop();
  }
  
  
  
  VkImageBlit info {};
  info.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  info.srcSubresource.baseArrayLayer = src_layer;
  info.srcSubresource.layerCount = 1;
  info.srcSubresource.mipLevel = 0;
  
  info.srcOffsets[0].x = 0;
  info.srcOffsets[0].y = 0;
  info.srcOffsets[0].z = 0;
  info.srcOffsets[1].x = src_tex->extent.width;
  info.srcOffsets[1].y = src_tex->extent.height;
  info.srcOffsets[1].z = src_tex->extent.depth;
  
  info.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  info.dstSubresource.baseArrayLayer = dst_layer;
  info.dstSubresource.layerCount = 1;
  info.dstSubresource.mipLevel = 0;
  
  info.dstOffsets[0].x = 0;
  info.dstOffsets[0].y = 0;
  info.dstOffsets[0].z = 0;
  info.dstOffsets[1].x = dst_tex->extent.width;
  info.dstOffsets[1].y = dst_tex->extent.height;
  info.dstOffsets[1].z = dst_tex->extent.depth;
  
  vkCmdBlitImage(command, src_tex->handle, src_tex->current_layout, dst_tex->handle, dst_tex->current_layout, 1, &info, VK_FILTER_NEAREST);
  
}



void detail_vk::frame_render::pre_draw(detail_vk::context& ctx_, const frontend::draw_command* draw) {
  
  
  
  
}

void detail_vk::frame_render::draw(detail_vk::context& ctx_, const frontend::draw_command* draw) {
  
}

}

