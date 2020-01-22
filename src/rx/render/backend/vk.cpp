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
  
  if (!create_instance(ctx)) return false;
  
  create_device(ctx);
  
  load_function_pointers(ctx);
  
  SET_NAME(ctx, VK_OBJECT_TYPE_INSTANCE, ctx.instance, "instance");
  
  SET_NAME(ctx, VK_OBJECT_TYPE_DEVICE, ctx.device, "device");
  
  ctx.graphics.init(ctx, ctx.graphics_index);
  ctx.transfer.init(ctx, ctx.graphics_index);
  
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
  
  ctx_.graphics.destroy(ctx_);
  ctx_.transfer.destroy(ctx_);
  
  //destroy_swapchain(ctx_);
  
  destroy_device(ctx_);
  
  destroy_instance(ctx_);
  
  ctx_.allocator->destroy<detail_vk::context>(m_impl);
  
}

void vk::process(const vector<rx_byte*>& _commands) {
  
  detail_vk::context& ctx{*reinterpret_cast<detail_vk::context*> (m_impl)};
  
  detail_vk::buffer_builder b_builder(ctx);
  detail_vk::texture_builder t_builder(ctx);
  
  detail_vk::frame_render frame;
  
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
      case frontend::command_type::k_resource_destroy: {
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
              create_swapchain(ctx);
              swap();
              break;
            }
            [[fallthrough]];
          default:
            t_builder.construct(ctx, resource);
        }
        break;
      }
      case frontend::command_type::k_resource_update: {
        const auto resource{reinterpret_cast<const frontend::resource_command*>(header + 1)};
        
        switch (resource->kind) {
          case frontend::resource_command::type::k_buffer:
            break;
          case frontend::resource_command::type::k_target:
            break;
          case frontend::resource_command::type::k_program:
            break;
          case frontend::resource_command::type::k_texture1D:
            break;
          case frontend::resource_command::type::k_texture2D:
            break;
          case frontend::resource_command::type::k_texture3D:
            break;
          case frontend::resource_command::type::k_textureCM:
            break;
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
      case frontend::command_type::k_profile: {
        break;
      }
    }
    
  });
  
  if(ctx.swap.alive) ctx.swap.image_info[ctx.swap.frame_index]->add_use(ctx, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, ctx.graphics_index, false);
  
  // end first pass
  
  
  // allocate memory
  b_builder.interpass(ctx);
  t_builder.interpass(ctx);
  
  
  
  ctx.graphics.start(ctx);
  ctx.transfer.start(ctx);
  
  // main process
  
  _commands.each_fwd([&](rx_byte* _command) {
    
    auto header {reinterpret_cast<frontend::command_header*>(_command)};
    ctx.current_command = header;
    switch(header->type) {
      
      case frontend::command_type::k_resource_allocate: {
        break;
      }
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
              destroy_swapchain(ctx);
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
      case frontend::command_type::k_resource_construct: {
        const auto resource{reinterpret_cast<const frontend::resource_command*>(header + 1)};
        
        switch (resource->kind) {
          case frontend::resource_command::type::k_buffer: {
            b_builder.construct2(ctx, resource->as_buffer);
            break;
          }
          case frontend::resource_command::type::k_target:
            reinterpret_cast<detail_vk::target*>(resource->as_target + 1)->construct(ctx, resource->as_target);
            break;
          case frontend::resource_command::type::k_program:
            
            break;
          case frontend::resource_command::type::k_texture2D:
            if(resource->as_texture2D->is_swapchain()) {
              break;
            }
            [[fallthrough]];
          default: {
            t_builder.construct2(ctx, resource);
            break;
          }
        }
        break;
      }
      case frontend::command_type::k_resource_update: {
        break;
      }
      case frontend::command_type::k_draw: {
        frame.draw(ctx, reinterpret_cast<const frontend::draw_command*>(header + 1));
        break;
      }
      case frontend::command_type::k_clear: {
        frame.clear(ctx, reinterpret_cast<const frontend::clear_command*>(header + 1));
        break;
      }
      case frontend::command_type::k_blit: {
        frame.blit(ctx, reinterpret_cast<const frontend::blit_command*>(header + 1));
        break;
      }
      case frontend::command_type::k_profile: {
        break;
      }
    }
    
  });
  
  if(ctx.swap.alive) {
    ctx.swap.image_info[ctx.swap.frame_index]->sync(ctx, ctx.swap.texture_info, ctx.graphics.get());
  }
  
  ctx.graphics.end(ctx, ctx.graphics_queue);
  ctx.transfer.end(ctx, ctx.graphics_queue);
  
}

void vk::process(rx_byte* _command) {
  (void) _command;
}

void vk::swap() {
  
  detail_vk::context& ctx{*reinterpret_cast<detail_vk::context*> (m_impl)};
  
  vk_log(log::level::k_verbose, "swap");
  
  if (!ctx.swap.alive) return;
  
  if (ctx.swap.acquired) {
  
    VkPresentInfoKHR info {};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.swapchainCount = 1;
    info.pSwapchains = &ctx.swap.swapchain;
    info.pImageIndices = &ctx.swap.frame_index;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &ctx.end_semaphore;
    
    check_result(vkQueuePresentKHR(ctx.graphics_queue, &info));
  
  }
  
  {
    
    check_result(vkAcquireNextImageKHR(ctx.device, ctx.swap.swapchain, 1000000000000L, ctx.end_semaphore, VK_NULL_HANDLE, &ctx.swap.frame_index));
    ctx.swap.acquired = true;
    
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
