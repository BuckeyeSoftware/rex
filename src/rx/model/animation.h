#ifndef RX_MODEL_ANIMATION_H
#define RX_MODEL_ANIMATION_H
#include "rx/core/vector.h"

#include "rx/math/vec2.h"
#include "rx/math/mat3x4.h"
#include "rx/math/mat4x4.h"
#include "rx/math/aabb.h"

namespace Rx::Model {

struct Loader;

struct Animation {
  Animation(Loader* _model, Size _index);

  void update(Float32 _delta_time, bool _loop);

  const Vector<Math::Mat3x4f>& frames() const &;
  const Math::AABB& bounds() const &;

  Size joints() const;

private:
  Loader* m_model;
  Vector<Math::Mat3x4f> m_frames;
  Size m_animation;
  Float32 m_current_frame;
  Math::AABB m_current_aabb;
  bool m_completed;
};

inline const Vector<Math::Mat3x4f>& Animation::frames() const & {
  return m_frames;
}

inline const Math::AABB& Animation::bounds() const & {
  return m_current_aabb;
}

} // namespace Rx::Model

#endif // RX_MODEL_ANIMATION
