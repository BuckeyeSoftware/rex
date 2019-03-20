#ifndef RX_RENDER_RENDERER_H
#define RX_RENDER_RENDERER_H

#include <rx/render/immediate.h>
#include <rx/render/frontend.h>
#include <rx/render/timer.h>

namespace rx::render {

struct frontend;
struct backend;

struct renderer {
  renderer(memory::allocator* _allocator, const char* _backend_name, void* _backend_data);
  ~renderer();
  
  bool update();

  target* back_buffer() const;
  target* composite_buffer() const;
  const frame_timer& timer() const;

  technique* find_technique_by_name(const char* _name);

private:
  backend* create_backend(const char* _name, void* _data);
  memory::allocator* create_allocator();

  memory::allocator* m_allocator;
  memory::allocator* m_frontend_allocator;
  memory::allocator* m_backend_allocator;

  backend* m_backend;

  frontend m_frontend;

  target* m_back_target;
  target* m_composite_target;

  immediate* m_immediates;

  map<string, technique> m_techniques;

  frame_timer m_timer;
};

inline const frame_timer& renderer::timer() const {
  return m_timer;
}

} // namespace rx::render

#endif // RX_RENDER_RENDERER_H