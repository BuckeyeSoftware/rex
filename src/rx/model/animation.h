#ifndef RX_MODEL_ANIMATION_H
#define RX_MODEL_ANIMATION_H
#include "rx/core/vector.h"

#include "rx/math/vec2.h"
#include "rx/math/mat3x4.h"
#include "rx/math/mat4x4.h"
#include "rx/math/aabb.h"
#include "rx/math/dual_quat.h"

namespace Rx::Model {

struct Loader;

struct Animation {
  Animation(const Loader* _model, Size _index);

  void update(Float32 _delta_time, bool _loop);

  const Vector<Math::Mat3x4f>& frames() const &;
  const Vector<Math::DualQuatf>& dq_frames() const { return m_dq_frames; }

  Size joints() const;

  Size index() const;

  // The two frame indices and the linear offset between them for interpolation.
  struct Interpolant {
    Size frame1;
    Size frame2;
    Float32 offset;
  };

  Interpolant interpolant() const;

private:
  const Loader* m_model;
  Vector<Math::Mat3x4f> m_frames;
  Vector<Math::DualQuatf> m_dq_frames;
  Size m_animation;
  Float32 m_current_frame;
  Interpolant m_interpolant;
  bool m_completed;
};

inline const Vector<Math::Mat3x4f>& Animation::frames() const & {
  return m_frames;
}

inline Size Animation::index() const {
  return m_animation;
}

inline Animation::Interpolant Animation::interpolant() const {
  return m_interpolant;
}

} // namespace Rx::Model

#endif // RX_MODEL_ANIMATION
