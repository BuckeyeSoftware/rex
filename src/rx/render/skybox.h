#ifndef RX_RENDER_SKYBOX_H
#define RX_RENDER_SKYBOX_H
#include "rx/math/vec2.h"
#include "rx/math/mat4x4.h"

#include "rx/core/string.h"
#include "rx/core/concurrency/spin_lock.h"

#include "rx/render/color_grader.h"

namespace Rx {
  struct Stream;
  struct JSON;
}

namespace Rx::Render {

namespace Frontend {
  struct Context;
  struct Technique;
  struct Texture;
  struct Texture2D;
  struct TextureCM;
  struct Target;
} // namespace Frontend

struct Skybox {
  Skybox(Frontend::Context* _frontend);
  ~Skybox();

  void render(Frontend::Target* _target, const Math::Mat4x4f& _view,
    const Math::Mat4x4f& _projection,
    const ColorGrader::Entry* _grading = nullptr);

  void load_async(const String& _file_name,
    const Math::Vec2z& _max_face_dimensions);

  bool load(Stream* _stream, const Math::Vec2z& _max_face_dimensions);
  bool load(const String& _file_name, const Math::Vec2z& _max_face_dimensions);

  Frontend::Texture* texture() const;

  const String& name() const &;

private:
  void release();

  Frontend::Texture2D* create_hdri(const JSON& _json) const;
  Frontend::TextureCM* create_cubemap(const JSON& _faces,
    const Math::Vec2z& _max_face_dimensions) const;

  Frontend::Context* m_frontend;
  Frontend::Technique* m_technique;

  Concurrency::Atomic<Frontend::Texture*> m_texture;
  String m_name;

  Concurrency::SpinLock m_lock;
};

inline Frontend::Texture* Skybox::texture() const {
  return m_texture;
}

inline const String& Skybox::name() const & {
  return m_name;
}

} // namespace Rx::Render

#endif // RX_RENDER_SKYBOX
