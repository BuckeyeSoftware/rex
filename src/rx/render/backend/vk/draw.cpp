#include "draw.h"

#include "context.h"

#include "data_builder.h"

namespace rx::render::backend {

void detail_vk::program::construct(detail_vk::context& ctx_, frontend::program* program) {
  
}

void detail_vk::program::destroy(detail_vk::context& ctx_, frontend::program* program) {
  
}

void detail_vk::target::construct(detail_vk::context& ctx_, frontend::target* target) {
  
  num_attachments = target->attachments().size() + (target->has_depth() || target->has_stencil());
  views = {ctx_.allocator, num_attachments};
  clears = {ctx_.allocator, num_attachments};
  
  for(rx_size i {0}; i<k_max_frames; i++) {
    framebuffers[i] = VK_NULL_HANDLE;
  }
  
}


void detail_vk::target::make_renderpass(detail_vk::context& ctx_, frontend::target* target) {
  
  bool swapchain = target->is_swapchain();
  
  VkRenderPassCreateInfo info {};
  info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  info.attachmentCount = target->attachments().size() + (target->has_depth() || target->has_stencil());
  
  rx::vector<VkAttachmentDescription> attachments (ctx_.allocator, info.attachmentCount);
  for (rx_size i {0}; i < target->attachments().size(); i++) {
    attachments[i] = {};
    attachments[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[i].samples = VK_SAMPLE_COUNT_1_BIT;
    
    frontend::texture* texture;
    if(target->attachments()[i].kind == frontend::target::attachment::type::k_textureCM) {
      auto CM = target->attachments()[i].as_textureCM.texture;
      texture = CM;
      attachments[i].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      attachments[i].format = reinterpret_cast<detail_vk::texture*>(CM + 1)->format;
    } else {
      auto t = target->attachments()[i].as_texture2D.texture;
      if(t->is_swapchain()) {
        attachments[i].format = ctx_.swap.image->format;
        
      } else {
        auto tex = reinterpret_cast<detail_vk::texture*>(t + 1);
        attachments[i].format = tex->format;
      }
      texture = t;
    }
    
    if(texture->is_swapchain()) {
      attachments[i].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    } else {
      attachments[i].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    
  }
  
  frontend::texture2D* ds = nullptr;
  if(target->has_depth_stencil()) {
    ds = target->depth_stencil();
  } else if(target->has_depth()) {
    ds = target->depth();
  } else if(target->has_stencil()) {
    ds = target->stencil();
  }
  if(ds != nullptr) {
    
    auto& att = attachments[attachments.size()-1];
    att = {};
    att.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    att.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    att.format = reinterpret_cast<detail_vk::texture*>(ds + 1)->format;
    att.samples = VK_SAMPLE_COUNT_1_BIT;
    att.loadOp = target->has_depth() ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    att.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    att.stencilLoadOp = target->has_stencil() ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    
  }
  
  info.pAttachments = attachments.data();
  
  info.subpassCount = 1;
  VkSubpassDescription subpass {};
  subpass.colorAttachmentCount = target->attachments().size();
  
  rx::vector<VkAttachmentReference> refs (ctx_.allocator, target->attachments().size());
  for (rx_size i {0}; i < target->attachments().size(); i++) {
    refs[i].attachment = i;
    refs[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  }
  subpass.pColorAttachments = refs.data();
  
  VkAttachmentReference depth_ref {};
  if(ds != nullptr) {
    depth_ref.attachment = attachments.size()-1;
    depth_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    subpass.pDepthStencilAttachment = &depth_ref;
  }
  
  subpass.inputAttachmentCount = 0;
  subpass.preserveAttachmentCount = 0;
  
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  info.pSubpasses = &subpass;
  
  vkCreateRenderPass(ctx_.device, &info, nullptr, &renderpass);
  
}

void detail_vk::target::make_framebuffer(detail_vk::context& ctx_, frontend::target* target) {
  
  bool swapchain = target->is_swapchain();
  
  int swapchain_image_index = -1;
  
  VkFramebufferCreateInfo info {};
  info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  info.width = target->dimensions().x;
  info.height = target->dimensions().y;
  info.layers = 1;
  info.renderPass = renderpass;
  info.attachmentCount = target->attachments().size() + (target->has_depth() || target->has_stencil());
  
  for (rx_size i {0}; i < target->attachments().size(); i++) {
    if(target->attachments()[i].kind == frontend::target::attachment::type::k_textureCM) {
      auto& CM = target->attachments()[i].as_textureCM;
      views[i] = reinterpret_cast<detail_vk::texture*>(CM.texture + 1)->make_attachment(ctx_, CM.texture, static_cast<uint8_t> (CM.face), target->attachments()[i].level);
    } else {
      auto& t = target->attachments()[i].as_texture2D;
      if(t.texture->is_swapchain()) {
        views[i] = VK_NULL_HANDLE;
        swapchain_image_index = i;
      } else {
        views[i] = reinterpret_cast<detail_vk::texture*>(t.texture + 1)->make_attachment(ctx_, t.texture, 0, target->attachments()[i].level);
      }
    }
  }
  
  frontend::texture2D* ds = nullptr;
  if(target->has_depth_stencil()) {
    ds = target->depth_stencil();
  } else if(target->has_depth()) {
    ds = target->depth();
  } else if(target->has_stencil()) {
    ds = target->stencil();
  }
  if(ds != nullptr) views[views.size()-1] = reinterpret_cast<detail_vk::texture*>(ds + 1)->make_attachment(ctx_, ds, 0, 0);
  
  info.pAttachments = views.data();
  
  if(swapchain) {
    for(rx_size i {0}; i<ctx_.swap.num_frames; i++) {
      views[swapchain_image_index] = ctx_.swap.image_views[i];
      vkCreateFramebuffer(ctx_.device, &info, nullptr, &framebuffers[i]);
    }
    views[swapchain_image_index] = VK_NULL_HANDLE;
  } else {
    vkCreateFramebuffer(ctx_.device, &info, nullptr, &framebuffers[0]);
  }
  
}


void detail_vk::target::make(detail_vk::context& ctx_, frontend::target* target) {
  
  make_renderpass(ctx_, target);
  
  if(framebuffers[0] == VK_NULL_HANDLE) {
    make_framebuffer(ctx_, target);
  }
  
}

VkFramebuffer detail_vk::target::get_framebuffer(detail_vk::context& ctx_, frontend::target* target) {
  
  if(target->is_swapchain()) {
    return framebuffers[ctx_.swap.frame_index];
  }
  return framebuffers[0];
  
}

VkRenderPass detail_vk::target::get_renderpass(detail_vk::context& ctx_, frontend::target* target) {
  
  return renderpass;
  
}

void detail_vk::target::start_renderpass(detail_vk::context& ctx_, frontend::target* target, VkCommandBuffer command) {
  
  
  
}

void detail_vk::target::end_renderpass(detail_vk::context& ctx_, frontend::target* target, VkCommandBuffer command) {
  
  
}

void detail_vk::target::clear(detail_vk::context& ctx_, frontend::target* target, const frontend::clear_command* clear) {
  
  
  
}


void detail_vk::target::destroy(detail_vk::context& ctx_, frontend::target* target) {
  
  if(renderpass != VK_NULL_HANDLE) {
    vkDestroyRenderPass(ctx_.device, renderpass, nullptr);
  }
  
  for(rx_size i {0}; i<framebuffers.size(); i++) {
    if(framebuffers[i] != VK_NULL_HANDLE) {
      vkDestroyFramebuffer(ctx_.device, framebuffers[i], nullptr);
    }
  }
  
  for(rx_size i {0}; i<views.size(); i++) {
    if(views[i] != VK_NULL_HANDLE) {
      vkDestroyImageView(ctx_.device, views[i], nullptr);
    }
  }
  
}



void detail_vk::pre_blit(detail_vk::context& ctx_, const frontend::blit_command* blit) {
  
  auto& src_texture = blit->src_target->attachments()[blit->src_attachment];
  auto src_tex = (src_texture.kind == frontend::target::attachment::type::k_textureCM ? reinterpret_cast<detail_vk::texture*>(src_texture.as_textureCM.texture + 1) : reinterpret_cast<detail_vk::texture*>(src_texture.as_texture2D.texture + 1));
  auto& dst_texture = blit->dst_target->attachments()[blit->dst_attachment];
  auto dst_tex = (dst_texture.kind == frontend::target::attachment::type::k_textureCM ? reinterpret_cast<detail_vk::texture*>(dst_texture.as_textureCM.texture + 1) : reinterpret_cast<detail_vk::texture*>(dst_texture.as_texture2D.texture + 1));
  
  src_tex->frame_uses.push(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, true, true, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT);
  dst_tex->frame_uses.push(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, true, true, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT);
  
}

void detail_vk::blit(detail_vk::context& ctx_, const frontend::blit_command* blit) {
  
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



void detail_vk::pre_draw(detail_vk::context& ctx_, const frontend::draw_command* draw) {
  
}

void detail_vk::draw(detail_vk::context& ctx_, const frontend::draw_command* draw) {
  
}

}

