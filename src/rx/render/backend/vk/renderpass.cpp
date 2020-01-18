#include "renderpass.h"

#include "context.h"
#include "data_builder.h"

namespace rx::render::backend {

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
        attachments[i].format = ctx_.swap.format;
        
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

}
