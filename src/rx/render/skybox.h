#ifndef RX_RENDER_SKYBOX_H
#define RX_RENDER_SKYBOX_H
#include "rx/math/mat4x4.h"
#include "rx/core/string.h"

namespace rx {
  struct stream;
}

namespace rx::render {

namespace frontend {
  struct context;
  struct technique;
  struct textureCM;
  struct target;
  struct buffer;
}

struct skybox {
  skybox(frontend::context* _frontend);
  ~skybox();

  void render(frontend::target* _target, const math::mat4x4f& _view,
    const math::mat4x4f& _projection);

  bool load(stream* _stream);
  bool load(const string& _file_name);

  frontend::textureCM* cubemap() const;

  const string& name() const &;

private:
  frontend::context* m_frontend;
  frontend::technique* m_technique;
  frontend::textureCM* m_texture;
  frontend::buffer* m_buffer;
  string m_name;
};

inline frontend::textureCM* skybox::cubemap() const {
  return m_texture;
}

inline const string& skybox::name() const & {
  return m_name;
}

} // namespace rx::render

#endif // RX_RENDER_SKYBOX
