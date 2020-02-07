#include "draw.h"

#include "context.h"

#include "sync.h"
#include "renderpass.h"

namespace rx::render::backend {

frontend::texture* detail_vk::get_texture (const frontend::target::attachment& attachment) {
  return (attachment.kind == frontend::target::attachment::type::k_textureCM ? static_cast<frontend::texture*> (attachment.as_textureCM.texture) : attachment.as_texture2D.texture);
}

detail_vk::texture* detail_vk::get_tex (detail_vk::context& ctx_, const frontend::target::attachment& attachment) {
  frontend::texture* dst_texture = get_texture(attachment);
  if(dst_texture->is_swapchain()) {
    return ctx_.swap.image_info[ctx_.swap.frame_index];
  } else {
    return (attachment.kind == frontend::target::attachment::type::k_textureCM ? reinterpret_cast<detail_vk::texture*> (attachment.as_textureCM.texture + 1) : reinterpret_cast<detail_vk::texture*> (attachment.as_texture2D.texture + 1));
  }
}


detail_vk::frame_render::frame_render(context& ctx_) : renderpasses(ctx_.allocator) {
  
  
  
}

void detail_vk::frame_render::pre_sync(detail_vk::context& ctx_, const frontend::target* target) {
  
  auto& renderpass = renderpasses.last();
  
  for(rx_size i {0}; i < target->attachments().size(); i++) {
    auto tex = get_tex(ctx_, target->attachments()[i]);
    renderpass.attachment_uses[i] = tex->add_use(ctx_, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT, ctx_.graphics_index, true, true);
  }
  
  frontend::texture2D* ds = nullptr;
  VkImageLayout layout{};
  if(target->has_depth_stencil()) {
    ds = target->depth_stencil();
    layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  } else if(target->has_depth()) {
    ds = target->depth();
    layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
  } else if(target->has_stencil()) {
    ds = target->stencil();
    layout = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
  }
  if(ds != nullptr) {
    renderpass.depth_stencil_use = reinterpret_cast<texture*>(ds+1)->add_use(ctx_, layout,
      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
      ctx_.graphics_index, true, true
    );
  }
  
}


void detail_vk::frame_render::pre_clear(detail_vk::context& ctx_, const frontend::clear_command* clear) {
  
  renderpasses.emplace_back(ctx_, clear->render_target);
  
  pre_sync(ctx_, clear->render_target);
  
}


void detail_vk::frame_render::pre_draw(context& ctx_, const frontend::draw_command* draw) {
  
  if(  renderpasses.size() == 0 // if first renderpass
    || renderpasses.last().target != draw->render_target // if target has changed
    || renderpasses.last().blits.size() > 0 // if this renderpass has already blitted, must change renderpass
  ) {
    renderpasses.emplace_back(ctx_, draw->render_target);
    
    pre_sync(ctx_, draw->render_target);
    
  }
  
  /*
  draw->render_target->attachments().each_fwd([&](const frontend::target::attachment& attachment) {
    auto tex = get_tex(ctx_, attachment);
    tex->add_use(ctx_, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, ctx_.graphics_index, true);
  });
  */
  
}


void detail_vk::frame_render::pre_blit(detail_vk::context& ctx_, const frontend::blit_command* blit) {
  
  if(renderpasses.size() == 0) {
    renderpasses.emplace_back(ctx_, nullptr);
  }
  
  auto& renderpass = renderpasses.last();
  
  
  auto src_tex = get_tex(ctx_, blit->src_target->attachments()[blit->src_attachment]);
  auto dst_tex = get_tex(ctx_, blit->dst_target->attachments()[blit->dst_attachment]);
  
  auto src_use = src_tex->add_use(ctx_, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, ctx_.graphics_index, false);
  auto dst_use = dst_tex->add_use(ctx_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, ctx_.graphics_index, true);
  
  renderpass.blits.push_back({blit, src_use, dst_use});
  
}



void detail_vk::frame_render::render(context& ctx_) {
  
  VkCommandBuffer command = ctx_.graphics.get(ctx_);
  
  renderpasses.each_fwd([&](frame_render::renderpass_info& renderpass) {
    
    if(renderpass.target != nullptr) {
      
      rx_size num_attachments = renderpass.target->attachments().size() + (renderpass.target->has_depth() || renderpass.target->has_stencil());
      
      auto targ = reinterpret_cast<target*>(renderpass.target+1);
      VkRenderPass rp = targ->make_renderpass(ctx_, renderpass);
      VkFramebuffer fb = targ->make_framebuffer(ctx_, renderpass.target, rp);
      
      {
        VkRenderPassBeginInfo info {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = rp;
        info.framebuffer = fb;
        info.renderArea.offset.x = 0;
        info.renderArea.offset.y = 0;
        info.renderArea.extent.width = renderpass.target->dimensions().x;
        info.renderArea.extent.height = renderpass.target->dimensions().y;
        
        if(renderpass.clear) {
          
          auto clear = renderpass.clear;
          
          rx::vector<VkClearValue> clears (ctx_.allocator, num_attachments);
          
          for (rx_u32 i{0}; i < sizeof (clear->color_values) / sizeof (*clear->color_values); i++) {
            if (clear->clear_colors & (1 << i)) {
              rx_size index = clear->draw_buffers.elements[i];
              
              VkClearValue c {};
              c.color.float32[0] = clear->color_values[i].r;
              c.color.float32[1] = clear->color_values[i].g;
              c.color.float32[2] = clear->color_values[i].b;
              c.color.float32[3] = clear->color_values[i].a;
              
              clears[index] = c;
              
            }
          }
          
          if(clear->clear_depth || clear->clear_stencil) {
            VkClearValue c {};
            c.depthStencil.depth = clear->depth_value;
            c.depthStencil.stencil = clear->stencil_value;
            
            clears[clears.size()-1] = c;
          }
          
          info.clearValueCount = clears.size();
          info.pClearValues = clears.data();
          
        }
        
        vkCmdBeginRenderPass(command, &info, VK_SUBPASS_CONTENTS_INLINE);
      }
      
      vkCmdEndRenderPass(command);
      
    }
    
    renderpass.blits.each_fwd([&](frame_render::renderpass_info::blit_info& info) {
      blit(ctx_, info);
    });
    
  });
  
}




void detail_vk::frame_render::blit(detail_vk::context& ctx_, frame_render::renderpass_info::blit_info& blit_info) {
  
  auto blit = blit_info.blit;
  
  #if defined(RX_DEBUG)
  vk_log(log::level::k_verbose, "blit %s", ctx_.current_command->tag.description);
  #endif
  
  auto& src = blit->src_target->attachments()[blit->src_attachment];
  frontend::texture* src_texture = get_texture(src);
  auto src_tex = get_tex(ctx_, src);
  uint32_t src_layer = (src.kind == frontend::target::attachment::type::k_textureCM ? static_cast<uint8_t> (src.as_textureCM.face) : 0);
  
  
  auto& dst = blit->dst_target->attachments()[blit->dst_attachment];
  frontend::texture* dst_texture = get_texture(dst);
  auto dst_tex = get_tex(ctx_, dst);
  uint32_t dst_layer = (dst.kind == frontend::target::attachment::type::k_textureCM ? static_cast<uint8_t> (dst.as_textureCM.face) : 0);
  
  VkCommandBuffer command = ctx_.graphics.get(ctx_);
  
  src_tex->sync(ctx_, src_texture, blit_info.src_use, command);
  
  dst_tex->sync(ctx_, dst_texture, blit_info.dst_use, command);
  
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
  
  vkCmdBlitImage(command, src_tex->handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst_tex->handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &info, VK_FILTER_NEAREST);
  
}


detail_vk::frame_render::renderpass_info::renderpass_info(context& ctx_, frontend::target* target) : target(target),
  attachment_uses(ctx_.allocator, target->attachments().size()),
  draws(ctx_.allocator),
  blits(ctx_.allocator)
  {}

}

