#ifndef RX_RENDER_MODEL_H
#define RX_RENDER_MODEL_H
#include "rx/render/frontend/material.h"

#include "rx/math/mat4x4.h"
#include "rx/math/aabb.h"

namespace rx::render {

namespace frontend {
  struct interface;
  struct technique;
  struct buffer;
  struct target;
}

struct model {
  model(frontend::interface* _frontend);
  ~model();

 struct mesh {
    rx_size offset;
    rx_size count;
    rx_size material;
    math::aabb bounds;
  };

  void render(frontend::target* _target, const math::mat4x4f& _model,
    const math::mat4x4f& _view, const math::mat4x4f& _projection);
  
  bool load(const string& _file_name);

private:
  frontend::interface* m_frontend;
  frontend::technique* m_technique;
  frontend::buffer* m_buffer;
  vector<frontend::material> m_materials;
  vector<mesh> m_opaque_meshes;
  vector<mesh> m_transparent_meshes;
};

} // namespace rx::render

#endif // RX_RENDER_MODEL_H