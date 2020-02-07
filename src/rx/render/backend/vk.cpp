#include "rx/render/backend/vk.h"

#include "vk/context.h"
#include "vk/data_builder.h"
#include "vk/helper.h"
#include "vk/init.h"
#include "vk/draw.h"
#include "vk/renderpass.h"
#include "vk/program.h"

#include <SDL_vulkan.h>

#include "rx/core/memory/allocator.h"
#include "rx/core/log.h"
#include "rx/core/assert.h"
#include "rx/console/variable.h"
#include "rx/console/interface.h"
#include "rx/core/algorithm/clamp.h"
#include "rx/core/math/ceil.h"

#include "rx/render/frontend/target.h"
#include "rx/render/frontend/program.h"
#include "rx/render/frontend/state.h"
#include "rx/render/frontend/command.h"

namespace rx::render::backend {

vk::vk(memory::allocator* _allocator, void* _data) {
  
  m_impl = _allocator->create<detail_vk::context>();
  detail_vk::context& ctx{*reinterpret_cast<detail_vk::context*> (m_impl)};
  
  ctx.allocator = _allocator;
  ctx.window = reinterpret_cast<SDL_Window*>(_data);
  
  ctx.buffer_allocations = {ctx.allocator};
  ctx.image_allocations = {ctx.allocator};
  ctx.staging_allocations = {ctx.allocator};
  ctx.staging_allocation_buffers = {ctx.allocator};
  
}


bool vk::init() {
  
  detail_vk::context& ctx{*reinterpret_cast<detail_vk::context*> (m_impl)};
  
  if (!detail_vk::create_instance(ctx)) return false;
  
  detail_vk::create_device(ctx);
  
  SET_NAME(ctx, VK_OBJECT_TYPE_INSTANCE, ctx.instance, "instance");
  
  SET_NAME(ctx, VK_OBJECT_TYPE_DEVICE, ctx.device, "device");
  
  ctx.graphics.init(ctx, ctx.graphics_index);
  ctx.transfer.init(ctx, ctx.graphics_index);
  
  VkSemaphoreCreateInfo info {};
  info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  
  for(rx_size i {0}; i<ctx.transfer_semaphore.size(); i++) {
    vkCreateSemaphore(ctx.device, &info, nullptr, &ctx.transfer_semaphore[i]);
    SET_NAME(ctx, VK_OBJECT_TYPE_SEMAPHORE, ctx.transfer_semaphore[i], rx::string::format (ctx.allocator, "transfer %i", i).data());
  }
  
  for(rx_size i {0}; i<ctx.swapchain_semaphore.size(); i++) {
    vkCreateSemaphore(ctx.device, &info, nullptr, &ctx.swapchain_semaphore[i]);
    SET_NAME(ctx, VK_OBJECT_TYPE_SEMAPHORE, ctx.swapchain_semaphore[i], rx::string::format (ctx.allocator, "swapchain %i", i).data());
  }
  
  return true;
  
}

vk::~vk() {
  
  detail_vk::context& ctx_{*reinterpret_cast<detail_vk::context*> (m_impl)};
  
  vk_log(log::level::k_info, "destroying vulkan backend");
  
  LOCAL_DEV_LOAD(vkDeviceWaitIdle)
  vkDeviceWaitIdle(ctx_.device);
  
  
  ctx_.buffer_allocations.each_fwd([&](detail_vk::context::allocation& alloc) {
    vkFreeMemory(ctx_.device, alloc.memory, nullptr);
  });
  ctx_.image_allocations.each_fwd([&](detail_vk::context::allocation& alloc) {
    vkFreeMemory(ctx_.device, alloc.memory, nullptr);
  });
  ctx_.staging_allocation_buffers.each_fwd([&](VkBuffer alloc) {
    vkDestroyBuffer(ctx_.device, alloc, nullptr);
  });
  ctx_.staging_allocations.each_fwd([&](detail_vk::context::allocation& alloc) {
    vkFreeMemory(ctx_.device, alloc.memory, nullptr);
  });
  
  for(rx_size i {0}; i<ctx_.transfer_semaphore.size(); i++) {
    vkDestroySemaphore(ctx_.device, ctx_.transfer_semaphore[i], nullptr);
  }
  
  for(rx_size i {0}; i<ctx_.swapchain_semaphore.size(); i++) {
    vkDestroySemaphore(ctx_.device, ctx_.swapchain_semaphore[i], nullptr);
  }
  
  ctx_.graphics.destroy(ctx_);
  ctx_.transfer.destroy(ctx_);
  
  //destroy_swapchain(ctx_);
  
  detail_vk::destroy_device(ctx_);
  
  detail_vk::destroy_instance(ctx_);
  
  ctx_.allocator->destroy<detail_vk::context>(m_impl);
  
}

void vk::process(const vector<rx_byte*>& _commands) {
  
  vk_log(log::level::k_verbose, "process");
  
  detail_vk::context& ctx{*reinterpret_cast<detail_vk::context*> (m_impl)};
  
  ctx.sync = ctx.allocator->create<detail_vk::resource_sync>(ctx);
  
  
  ctx.index = (ctx.index + 1) % k_buffered;
  
  
  
  detail_vk::buffer_builder b_builder(ctx);
  detail_vk::texture_builder t_builder(ctx);
  
  detail_vk::frame_render frame(ctx);
  
  // pre process
  _commands.each_fwd([&](rx_byte* _command) {
    
    auto header {reinterpret_cast<const frontend::command_header*>(_command)};
    ctx.current_command = header;
    switch (header->type) {
      
      case frontend::command_type::k_resource_allocate: {
        const auto resource{reinterpret_cast<const frontend::resource_command*>(header + 1)};
        
        switch (resource->kind) {
          case frontend::resource_command::type::k_buffer:
            utility::construct<detail_vk::buffer>(resource->as_buffer + 1);
            break;
          case frontend::resource_command::type::k_target:
            utility::construct<detail_vk::target>(resource->as_target + 1);
            break;
          case frontend::resource_command::type::k_program:
            utility::construct<detail_vk::program>(resource->as_program + 1);
            break;
          case frontend::resource_command::type::k_texture1D:
            utility::construct<detail_vk::texture>(resource->as_texture1D + 1);
            break;
          case frontend::resource_command::type::k_texture2D:
            utility::construct<detail_vk::texture>(resource->as_texture2D + 1);
            break;
          case frontend::resource_command::type::k_texture3D:
            utility::construct<detail_vk::texture>(resource->as_texture3D + 1);
            break;
          case frontend::resource_command::type::k_textureCM:
            utility::construct<detail_vk::texture>(resource->as_textureCM + 1);
            break;
        }
        break;
      }
      case frontend::command_type::k_resource_construct: {
        const auto resource{reinterpret_cast<const frontend::resource_command*>(header + 1)};
        
        switch (resource->kind) {
          case frontend::resource_command::type::k_buffer: {
            b_builder.construct(ctx, resource->as_buffer);
            break;
          }
          case frontend::resource_command::type::k_target: {
            break;
          }
          case frontend::resource_command::type::k_program:
            reinterpret_cast<detail_vk::program*>(resource->as_program + 1)->construct(ctx, resource->as_program);
            break;
          case frontend::resource_command::type::k_texture2D:
            if(resource->as_texture2D->is_swapchain()) {
              ctx.swap.texture_info = resource->as_texture2D;
              detail_vk::create_swapchain(ctx);
              break;
            }
            [[fallthrough]];
          default:
            t_builder.construct(ctx, resource);
        }
        break;
      }
      case frontend::command_type::k_draw: {
        frame.pre_draw(ctx, reinterpret_cast<const frontend::draw_command*>(header + 1));
        break;
      }
      case frontend::command_type::k_clear: {
        frame.pre_clear(ctx, reinterpret_cast<const frontend::clear_command*>(header + 1));
        break;
      }
      case frontend::command_type::k_blit: {
        frame.pre_blit(ctx, reinterpret_cast<const frontend::blit_command*>(header + 1));
        break;
      }
      default:
        break;
    }
    
  });
  
  detail_vk::use_queue::use_info* swap_present_use = ctx.swap.alive ? ctx.swap.image_info[ctx.swap.frame_index]->add_use(ctx, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, ctx.graphics_index, false) : nullptr;
  
  // end first pass
  
  
  
  // allocate memory and transfer ops
  
  ctx.transfer.start(ctx);
  
  b_builder.build(ctx);
  t_builder.build(ctx);
  
  {
    rx::vector<VkSemaphore> signals(ctx.allocator);
    signals.push_back(ctx.transfer_semaphore[ctx.index]);
    ctx.transfer.end(ctx, ctx.graphics_queue, {}, {}, signals);
  }
  
  // main render process
  
  ctx.graphics.start(ctx);
  
  frame.render(ctx);
  
  if(ctx.swap.alive) ctx.swap.image_info[ctx.swap.frame_index]->sync(ctx, ctx.swap.texture_info, swap_present_use, ctx.graphics.get(ctx));
  
  ctx.sync->clear(ctx);
  
  ctx.allocator->destroy<detail_vk::resource_sync>(ctx.sync);
  
  
  {
    auto waits = rx::vector<VkSemaphore> (ctx.allocator);
    auto stages = rx::vector<VkPipelineStageFlags> (ctx.allocator);
    waits.push_back(ctx.transfer_semaphore[ctx.index]);
    stages.push_back(VK_PIPELINE_STAGE_TRANSFER_BIT);
    
    if(ctx.swapchain_sync_index != -1) {
      waits.push_back(ctx.swapchain_semaphore[ctx.swapchain_sync_index]);
      stages.push_back(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    }
    
    
    vk_log(log::level::k_verbose, "render");
    
    ctx.swapchain_sync_index = (ctx.swapchain_sync_index + 1)%ctx.swapchain_semaphore.size();
    
    rx::vector<VkSemaphore> signals(ctx.allocator);
    signals.push_back(ctx.swapchain_semaphore[ctx.swapchain_sync_index]);
    
    ctx.graphics.end(ctx, ctx.graphics_queue, waits, stages, signals);
  }
  
  
  
  _commands.each_fwd([&](rx_byte* _command) {
    
    auto header {reinterpret_cast<frontend::command_header*>(_command)};
    ctx.current_command = header;
    switch(header->type) {
      case frontend::command_type::k_resource_destroy: {
        const auto resource{reinterpret_cast<const frontend::resource_command*>(header + 1)};
        
        switch (resource->kind) {
          case frontend::resource_command::type::k_buffer:
            reinterpret_cast<detail_vk::buffer*>(resource->as_buffer + 1)->destroy(ctx, resource->as_buffer);
            utility::destruct<detail_vk::buffer>(resource->as_buffer + 1);
            break;
          case frontend::resource_command::type::k_target: {
            reinterpret_cast<detail_vk::target*>(resource->as_target + 1)->destroy(ctx, resource->as_target);
            utility::destruct<detail_vk::target>(resource->as_target + 1);
            break;
          }
          case frontend::resource_command::type::k_program:
            reinterpret_cast<detail_vk::program*>(resource->as_program + 1)->destroy(ctx, resource->as_program);
            utility::destruct<detail_vk::program>(resource->as_program + 1);
            break;
          case frontend::resource_command::type::k_texture1D:
            reinterpret_cast<detail_vk::texture*>(resource->as_texture1D + 1)->destroy(ctx, resource->as_texture1D);
            utility::destruct<detail_vk::texture>(resource->as_texture1D + 1);
            break;
          case frontend::resource_command::type::k_texture2D:
            if(resource->as_texture2D->is_swapchain()) {
              detail_vk::destroy_swapchain(ctx);
            } else {
              reinterpret_cast<detail_vk::texture*>(resource->as_texture2D + 1)->destroy(ctx, resource->as_texture2D);
            }
            utility::destruct<detail_vk::texture>(resource->as_texture2D + 1);
            break;
          case frontend::resource_command::type::k_texture3D:
            reinterpret_cast<detail_vk::texture*>(resource->as_texture3D + 1)->destroy(ctx, resource->as_texture3D);
            utility::destruct<detail_vk::texture>(resource->as_texture3D + 1);
            break;
          case frontend::resource_command::type::k_textureCM:
            reinterpret_cast<detail_vk::texture*>(resource->as_textureCM + 1)->destroy(ctx, resource->as_textureCM);
            utility::destruct<detail_vk::texture>(resource->as_textureCM + 1);
            break;
        }
        break;
      }
      default:
        break;
    }
    
  });
  
  
  ctx.index = (ctx.index + 1) % k_buffered;
  
  
}

void vk::process(rx_byte* _command) {
  (void) _command;
}

void vk::swap() {
  
  detail_vk::context& ctx{*reinterpret_cast<detail_vk::context*> (m_impl)};
  
  if (!ctx.swap.alive) return;
  
  {
    vk_log(log::level::k_verbose, "present %i", ctx.swapchain_sync_index);
    
    VkPresentInfoKHR info {};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.swapchainCount = 1;
    info.pSwapchains = &ctx.swap.swapchain;
    info.pImageIndices = &ctx.swap.frame_index;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &ctx.swapchain_semaphore[ctx.swapchain_sync_index];
    
    check_result(vkQueuePresentKHR(ctx.graphics_queue, &info));
  }
  
  ctx.swapchain_sync_index = (ctx.swapchain_sync_index + 1)%ctx.swapchain_semaphore.size();
  
  {
    vk_log(log::level::k_verbose, "acquire %i", ctx.swapchain_sync_index);
    check_result(vkAcquireNextImageKHR(ctx.device, ctx.swap.swapchain, 1000000000000L, ctx.swapchain_semaphore[ctx.swapchain_sync_index], VK_NULL_HANDLE, &ctx.swap.frame_index));
  }
  
}

allocation_info vk::query_allocation_info() const {
  allocation_info info {};
  info.buffer_size = sizeof(detail_vk::buffer);
  info.program_size = sizeof(detail_vk::program);
  info.target_size = sizeof(detail_vk::target);
  info.texture1D_size = sizeof(detail_vk::texture);
  info.texture2D_size = sizeof(detail_vk::texture);
  info.texture3D_size = sizeof(detail_vk::texture);
  info.textureCM_size = sizeof(detail_vk::texture);
  return info;
}

device_info vk::query_device_info() const {
  device_info info {};
  info.renderer = "";
  info.vendor = "";
  info.version = "";
  return info;
}


}
