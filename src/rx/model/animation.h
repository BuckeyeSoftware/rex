#ifndef RX_MODEL_ANIMATION_H
#define RX_MODEL_ANIMATION_H
#include "rx/core/vector.h"
#include "rx/core/string.h"

#include "rx/math/vec2.h"
#include "rx/math/mat3x4.h"
#include "rx/math/mat4x4.h"
#include "rx/math/aabb.h"
#include "rx/math/dual_quat.h"

namespace Rx::Model {

struct Loader;
struct Skeleton;

struct Clip {
  Size index;
  Float32 frame_rate;
  Size frame_offset;
  Size frame_count;
  String name;
};

struct Animation {
  Animation(const Skeleton* _skeleton, const Clip* _clip);

  void update(Float32 _delta_time, bool _loop);

  const Vector<Math::Mat3x4f>& lb_frames() const & { return m_rendered_lb_frames; }
  const Vector<Math::DualQuatf>& dq_frames() const & { return m_rendered_dq_frames; }

  // The two frame indices and the linear offset between them for interpolation.
  struct Interpolant {
    Size frame1;
    Size frame2;
    Float32 offset;
  };

  Interpolant interpolant() const;

  const Skeleton* skeleton() const;
  const Clip* clip() const;

private:
  const Skeleton* m_skeleton;
  const Clip* m_clip;

  Vector<Math::Mat3x4f> m_rendered_lb_frames;
  Vector<Math::DualQuatf> m_rendered_dq_frames;
  Float32 m_current_frame;
  Interpolant m_interpolant;
  bool m_completed;
};

inline Animation::Interpolant Animation::interpolant() const {
  return m_interpolant;
}

inline const Skeleton* Animation::skeleton() const {
  return m_skeleton;
}

inline const Clip* Animation::clip() const {
  return m_clip;
}

} // namespace Rx::Model

#endif // RX_MODEL_ANIMATION
