#ifndef RX_MODEL_SKELETON_H
#define RX_MODEL_SKELETON_H
#include "rx/core/vector.h"

#include "rx/math/mat3x4.h"
#include "rx/math/dual_quat.h"

namespace Rx::Model {

struct Skeleton {
  RX_MARK_NO_COPY(Skeleton);

  constexpr Skeleton() = default;
  Skeleton(Skeleton&& skeleton_);
  Skeleton& operator=(Skeleton&& skeleton_);

  struct Joint {
    Math::Mat3x4f frame;
    Sint32 parent;
  };

  static Optional<Skeleton> create(Vector<Joint>&& joints_, Vector<Math::Mat3x4f>&& frames_);
  static Optional<Skeleton> copy(const Skeleton& _skeleton);

  void transform(const Math::Mat3x4f& _transform);

  const Vector<Joint>& joints() const & { return m_joints; }
  const Vector<Math::Mat3x4f>& lb_frames() const & { return m_lb_frames; }
  const Vector<Math::DualQuatf>& dq_frames() const & { return m_dq_frames; }

private:
  Vector<Joint> m_joints;
  Vector<Math::Mat3x4f> m_lb_frames;
  Vector<Math::DualQuatf> m_dq_frames;
};

inline Skeleton::Skeleton(Skeleton&& skeleton_)
  : m_joints{Utility::move(skeleton_.m_joints)}
  , m_lb_frames{Utility::move(skeleton_.m_lb_frames)}
  , m_dq_frames{Utility::move(skeleton_.m_dq_frames)}
{
}

inline Skeleton& Skeleton::operator=(Skeleton&& skeleton_) {
  if (&skeleton_ != this) {
    m_joints = Utility::move(skeleton_.m_joints);
    m_lb_frames = Utility::move(skeleton_.m_lb_frames);
    m_dq_frames = Utility::move(skeleton_.m_dq_frames);
  }
  return *this;
}

} // namespace Rx::Model

#endif // RX_MODEL_SKELETON_H