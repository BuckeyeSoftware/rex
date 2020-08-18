#ifndef RX_RENDER_SKYBOX_H
#define RX_RENDER_SKYBOX_H
#include "rx/math/mat4x4.h"
#include "rx/core/string.h"

namespace Rx {
  struct Stream;
}

namespace Rx::Render {

namespace Frontend {
  struct Context;
  struct Technique;
  struct TextureCM;
  struct Target;
}

struct Skybox {
  Skybox(Frontend::Context* _frontend);
  ~Skybox();

  void render(Frontend::Target* _target, const Math::Mat4x4f& _view,
              const Math::Mat4x4f& _projection);

  bool load(Stream* _stream);
  bool load(const String& _file_name);

  Frontend::TextureCM* cubemap() const;

  const String& name() const &;

private:
  Frontend::Context* m_frontend;
  Frontend::Technique* m_technique;
  Frontend::TextureCM* m_texture;
  String m_name;
};

inline Frontend::TextureCM* Skybox::cubemap() const {
  return m_texture;
}

inline const String& Skybox::name() const & {
  return m_name;
}

} // namespace rx::render

#endif // RX_RENDER_SKYBOX
