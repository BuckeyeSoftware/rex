#include "rx/model/skeleton.h"

namespace Rx::Model {

void Skeleton::transform(const Math::Mat3x4f& _transform) {
  const auto inverse = Math::invert(_transform);

  const auto n_frames = m_lb_frames.size();

  m_lb_frames.each_fwd([&](Math::Mat3x4f& frame_) {
    frame_ = _transform * frame_ * inverse;
  });

  // TODO(dweiler): Transform the DQ frames instead of recreating them.
  for (Size i = 0; i < n_frames; i++) {
    m_dq_frames[i] = m_lb_frames[i];
    m_dq_frames[i].real = Math::normalize(m_dq_frames[i].real);
  }

  m_joints.each_fwd([&](Joint& joint_) {
    joint_.frame = _transform * joint_.frame * inverse;
  });
}

Optional<Skeleton> Skeleton::create(Vector<Joint>&& joints_, Vector<Math::Mat3x4f>&& frames_) {
  const auto n_frames = frames_.size();

  auto& allocator = frames_.allocator();
  Vector<Math::DualQuatf> dq_frames{allocator};
  if (!dq_frames.resize(n_frames)) {
    return nullopt;
  }

  for (Size i = 0; i < n_frames; i++) {
    dq_frames[i] = frames_[i];
    dq_frames[i].real = Math::normalize(dq_frames[i].real);
  }

  return Skeleton {
    Utility::move(joints_),
    Utility::move(frames_),
    Utility::move(dq_frames)
  };
}

Optional<Skeleton> Skeleton::copy(const Skeleton& _skeleton) {
  auto joints = Utility::copy(_skeleton.m_joints);
  auto lb_frames = Utility::copy(_skeleton.m_lb_frames);
  auto dq_frames = Utility::copy(_skeleton.m_dq_frames);

  if (!joints || !lb_frames || !dq_frames) {
    return nullopt;
  }

  return Skeleton {
    Utility::move(*joints),
    Utility::move(*lb_frames),
    Utility::move(*dq_frames)
  };
}

} // namespace Rx::Model