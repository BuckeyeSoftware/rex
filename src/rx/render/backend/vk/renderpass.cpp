#include "renderpass.h"

#include "context.h"
#include "data_builder.h"

namespace rx::render::backend {

void detail_vk::target::construct(detail_vk::context& ctx_, frontend::target* target) {
  
  (void) ctx_;
  (void) target;
  
}


VkRenderPass detail_vk::target::make_renderpass(detail_vk::context& ctx_, frame_render::renderpass_info& rpinfo) {
  
  auto target = rpinfo.target;
  
  rx_size num_attachments = target->attachments().size() + (target->has_depth() || target->has_stencil());
  
  VkRenderPassCreateInfo info {};
  info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  info.attachmentCount = num_attachments;
  
  rx::vector<VkSubpassDependency> dependencies (ctx_.allocator);
  
  
  rx::vector<VkAttachmentDescription> attachments (ctx_.allocator, num_attachments);
  for (rx_size i {0}; i < target->attachments().size(); i++) {
    
    frontend::texture* texture = get_texture(target->attachments()[i]);
    detail_vk::texture* tex = get_tex(ctx_, target->attachments()[i]);
    
    attachments[i] = {};
    
    //TODO: change this with sync_after
    
    auto last_use = rpinfo.attachment_uses[i];
    auto current_use = last_use->after;
    
    
    attachments[i].initialLayout = last_use->layout;
    
    
    if(last_use->queue != current_use->queue) {
      
      tex->sync(ctx_, texture, last_use, ctx_.graphics.get(ctx_));
      
    } else if(!last_use->sync_after && last_use->write) {
      
      // TODO: add dependency
      
    }
    
    if(current_use->after != nullptr) {
      attachments[i].finalLayout = current_use->after->layout;
      attachments[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    } else {
      attachments[i].finalLayout = current_use->layout;
      attachments[i].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    }
    
    attachments[i].loadOp = (last_use->layout == VK_IMAGE_LAYOUT_UNDEFINED) ? VK_ATTACHMENT_LOAD_OP_DONT_CARE : VK_ATTACHMENT_LOAD_OP_LOAD;
    
    attachments[i].format = tex->format;
    attachments[i].samples = VK_SAMPLE_COUNT_1_BIT;
    
    attachments[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    
  }
  
  if (rpinfo.clear && rpinfo.clear->clear_colors) {
    for (rx_u32 i{0}; i < sizeof rpinfo.clear->color_values / sizeof *rpinfo.clear->color_values; i++) {
      if (rpinfo.clear->clear_colors & (1 << i)) {
        rx_size index = rpinfo.clear->draw_buffers.elements[i];
        
        attachments[index].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[index].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        
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
  
  if(ds != nullptr) {
    
    auto tex = reinterpret_cast<detail_vk::texture*>(ds + 1);
    
    auto& att = attachments[attachments.size()-1];
    att = {};
    att.format = tex->format;
    att.samples = VK_SAMPLE_COUNT_1_BIT;
    
    
    auto last_use = rpinfo.depth_stencil_use;
    auto current_use = last_use->after;
    
    att.initialLayout = last_use->layout;
    
    
    if(last_use->queue != current_use->queue) {
      
      tex->sync(ctx_, ds, last_use, ctx_.graphics.get(ctx_));
      
    } else if(!last_use->sync_after && last_use->write) {
      
      // TODO: add dependency
      
    }
    
    att.loadOp = (last_use->layout == VK_IMAGE_LAYOUT_UNDEFINED || !target->has_depth()) ? VK_ATTACHMENT_LOAD_OP_DONT_CARE : VK_ATTACHMENT_LOAD_OP_LOAD;
    att.stencilLoadOp = (last_use->layout == VK_IMAGE_LAYOUT_UNDEFINED || !target->has_stencil()) ? VK_ATTACHMENT_LOAD_OP_DONT_CARE : VK_ATTACHMENT_LOAD_OP_LOAD;
    
    if(rpinfo.clear && rpinfo.clear->clear_depth) att.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    if(rpinfo.clear && rpinfo.clear->clear_stencil) att.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    
    
    if(current_use->after != nullptr) {
      att.finalLayout = current_use->after->layout;
      att.storeOp = target->has_depth() ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
      att.stencilStoreOp = target->has_stencil() ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
    } else {
      att.finalLayout = current_use->layout;
      att.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    }
    
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
    if(target->has_depth_stencil()) {
      depth_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    } else if(target->has_depth()) {
      depth_ref.layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    } else if(target->has_stencil()) {
      depth_ref.layout = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
    }
    subpass.pDepthStencilAttachment = &depth_ref;
  }
  
  subpass.inputAttachmentCount = 0;
  subpass.preserveAttachmentCount = 0;
  
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  
  
  info.pSubpasses = &subpass;
  
  info.dependencyCount = dependencies.size();
  info.pDependencies = dependencies.data();
  
  VkRenderPass ret;
  check_result(vkCreateRenderPass(ctx_.device, &info, nullptr, &ret));
  
  return ret;
  
}

VkFramebuffer detail_vk::target::make_framebuffer(detail_vk::context& ctx_, frontend::target* target, VkRenderPass renderpass) {
  
  int swapchain_image_index = -1;
  
  VkFramebufferCreateInfo info {};
  info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  info.width = target->dimensions().x;
  info.height = target->dimensions().y;
  info.layers = 1;
  info.renderPass = renderpass;
  info.attachmentCount = target->attachments().size() + (target->has_depth() || target->has_stencil());
  
  rx::vector<VkImageView> views(ctx_.allocator, info.attachmentCount);
  
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
  
  
  if(swapchain_image_index != -1) {
    views[swapchain_image_index] = ctx_.swap.image_views[ctx_.swap.frame_index];
  }
  
  VkFramebuffer ret;
  check_result(vkCreateFramebuffer(ctx_.device, &info, nullptr, &ret));
  
  return ret;
}


void detail_vk::target::destroy(detail_vk::context& ctx_, frontend::target* target) {
  
}

}
