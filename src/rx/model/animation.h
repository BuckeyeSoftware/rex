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
  RX_MARK_NO_COPY(Animation);

  // The two frame indices and the linear offset between them for interpolation.
  struct Interpolant {
    Size frame1 = 0;
    Size frame2 = 0;
    Float32 offset = 0;
  };

  constexpr Animation();
  Animation(Animation&& animation_);
  Animation& operator=(Animation&& animation_);

  static Optional<Animation> create(const Skeleton& _skeleton, const Clip& _clip);

  void update(Float32 _delta_time, bool _loop);

  Interpolant interpolant() const;

  const Vector<Math::Mat3x4f>& lb_frames() const &;
  const Vector<Math::DualQuatf>& dq_frames() const &;

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

inline constexpr Animation::Animation()
  : m_skeleton{nullptr}
  , m_clip{nullptr}
  , m_current_frame{0.0f}
  , m_completed{false}
{
}

inline Animation::Animation(Animation&& animation_)
  : m_skeleton{Utility::exchange(animation_.m_skeleton, nullptr)}
  , m_clip{Utility::exchange(animation_.m_clip, nullptr)}
  , m_rendered_lb_frames{Utility::move(animation_.m_rendered_lb_frames)}
  , m_rendered_dq_frames{Utility::move(animation_.m_rendered_dq_frames)}
  , m_current_frame{Utility::exchange(animation_.m_current_frame, 0.0f)}
  , m_interpolant{Utility::move(animation_.m_interpolant)}
  , m_completed{Utility::exchange(animation_.m_completed, false)}
{
}

inline Animation& Animation::operator=(Animation&& animation_) {
  if (this == &animation_) {
    return *this;
  }

  m_skeleton = Utility::exchange(animation_.m_skeleton, nullptr);
  m_clip = Utility::exchange(animation_.m_clip, nullptr);
  m_rendered_lb_frames = Utility::move(animation_.m_rendered_lb_frames);
  m_rendered_dq_frames = Utility::move(animation_.m_rendered_dq_frames);
  m_current_frame = Utility::exchange(animation_.m_current_frame, 0);
  m_interpolant = Utility::move(animation_.m_interpolant);
  m_completed = Utility::exchange(animation_.m_completed, false);

  return *this;
}

inline Animation::Interpolant Animation::interpolant() const {
  return m_interpolant;
}

inline const Vector<Math::Mat3x4f>& Animation::lb_frames() const & {
  return m_rendered_lb_frames;
}

inline const Vector<Math::DualQuatf>& Animation::dq_frames() const & {
  return m_rendered_dq_frames;
}

inline const Skeleton* Animation::skeleton() const {
  return m_skeleton;
}

inline const Clip* Animation::clip() const {
  return m_clip;
}

} // namespace Rx::Model

#endif // RX_MODEL_ANIMATION
