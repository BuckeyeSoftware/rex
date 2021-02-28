#include <stdio.h>

#include "rx/model/skeleton.h"

namespace Rx::Model {

void Skeleton::transform(const Math::Mat3x4f& _transform) {
  const auto inverse = Math::Mat3x4f::invert(_transform);

  m_lb_frames.each_fwd([&](Math::Mat3x4f& frame_) {
    frame_ = _transform * frame_ * inverse;
  });

  // TODO(dweiler): Transform the DQ frames.

  m_joints.each_fwd([&](Joint& joint_) {
    joint_.frame = _transform * joint_.frame * inverse;
  });
}

Optional<Skeleton> Skeleton::create(Vector<Joint>&& joints_, Vector<Math::Mat3x4f>&& frames_) {
  const auto n_frames = frames_.size();

  Vector<Math::DualQuatf> dq_frames;
  if (!dq_frames.resize(n_frames)) {
    return nullopt;
  }

  for (Size i = 0; i < n_frames; i++) {
    dq_frames[i] = frames_[i];
    dq_frames[i].real = Math::normalize(dq_frames[i].real);
  }

  Skeleton result;
  result.m_dq_frames = Utility::move(dq_frames);
  result.m_lb_frames = Utility::move(frames_);
  result.m_joints = Utility::move(joints_);

  return result;
}

Optional<Skeleton> Skeleton::copy(const Skeleton& _skeleton) {
  auto joints = Utility::copy(_skeleton.m_joints);
  auto lb_frames = Utility::copy(_skeleton.m_lb_frames);
  auto dq_frames = Utility::copy(_skeleton.m_dq_frames);
  if (!joints || !lb_frames || !dq_frames) {
    return nullopt;
  }

  Skeleton result;
  result.m_joints = Utility::move(*joints);
  result.m_lb_frames = Utility::move(*lb_frames);
  result.m_dq_frames = Utility::move(*dq_frames);
  return result;
}

} // namespace Rx::Model