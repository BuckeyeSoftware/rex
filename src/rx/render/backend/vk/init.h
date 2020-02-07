#ifndef RX_RENDER_BACKEND_VK_INIT_H
#define RX_RENDER_BACKEND_VK_INIT_H

namespace rx::render::backend {
  
namespace detail_vk {
  struct context;

  bool create_instance(detail_vk::context& ctx_);

  void destroy_instance(detail_vk::context& ctx_);

  void create_device(detail_vk::context& ctx_);

  void destroy_device(detail_vk::context& ctx_);

  void create_swapchain(detail_vk::context& ctx_);

  void destroy_swapchain(detail_vk::context& ctx_);

  void load_function_pointers(detail_vk::context& ctx_);
}

}

#endif
