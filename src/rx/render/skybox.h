#ifndef RX_RENDER_SKYBOX_H
#define RX_RENDER_SKYBOX_H
#include "rx/math/mat4x4.h"

namespace rx {
  struct string;
}

namespace rx::render {

namespace frontend {
  struct interface;
  struct technique;
  struct textureCM;
  struct target;
  struct buffer;
}

struct skybox {
  skybox(frontend::interface* _interface);
  ~skybox();

  void render(frontend::target* _target, const math::mat4x4f& _view,
    const math::mat4x4f& _projection);

  bool load(const string& _file_name);

private:
  frontend::interface* m_frontend;
  frontend::technique* m_technique;
  frontend::textureCM* m_texture;
  frontend::buffer* m_buffer;
};

} // namespace rx::render

#endif // RX_RENDER_SKYBOX