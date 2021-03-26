#include "rx/render/immediate3D.h"

#include "rx/model/animation.h"
#include "rx/model/loader.h"
#include "rx/model/skeleton.h"

#include "rx/core/math/abs.h"
#include "rx/core/math/mod.h"

namespace Rx::Model {

Optional<Animation> Animation::create(Memory::Allocator& _allocator,
  Skeleton& _skeleton, const Clip& _clip)
{
  Animation result{_allocator};

  result.m_skeleton = &_skeleton;
  result.m_clip = &_clip;

  if (!result.m_rendered_lb_frames.resize(_skeleton.lb_frames().size())) {
    return nullopt;
  }

  if (!result.m_rendered_dq_frames.resize(_skeleton.dq_frames().size())) {
    return nullopt;
  }

  return result;
}

void Animation::update(Float32 _delta_time, bool _loop) {
  if (m_completed) {
    return;
  }

  m_current_frame += m_clip->frame_rate * _delta_time;

  const bool completes = m_current_frame >= m_clip->frame_count - 1;
  const bool finished = completes && !_loop;

  m_current_frame = Math::mod(m_current_frame, static_cast<Float32>(m_clip->frame_count));

  Size frame1 = finished ? m_clip->frame_count - 1 : static_cast<Size>(m_current_frame);
  Size frame2 = finished ? m_clip->frame_count - 1 : frame1 + 1;

  frame1 %= m_clip->frame_count;
  frame2 %= m_clip->frame_count;

  Math::Vec2z frame_indices;
  frame_indices[0] = m_clip->frame_offset + frame1;
  frame_indices[1] = m_clip->frame_offset + frame2;

  const Float32 offset =
          finished ? 0.0f : Math::abs(m_current_frame - frame1);

  m_interpolant.frame1 = frame1;
  m_interpolant.frame2 = frame2;
  m_interpolant.offset = offset;

  const auto& lb_frames = m_skeleton->lb_frames();
  const auto& dq_frames = m_skeleton->dq_frames();

  const auto n_joints = m_skeleton->joints().size();

  const auto* mat1 = &lb_frames[frame_indices[0] * n_joints];
  const auto* mat2 = &lb_frames[frame_indices[1] * n_joints];

  const auto* dq1 = &dq_frames[frame_indices[0] * n_joints];
  const auto* dq2 = &dq_frames[frame_indices[1] * n_joints];

  // Interpolate matrices between the two closest frames.
  for (Size i = 0; i < n_joints; i++) {
    m_rendered_lb_frames[i] = mat1[i] * (1.0f - offset) + mat2[i] * offset;
  }

  // Interpolate DQs between the two closest frames.
  for (Size i = 0; i < n_joints; i++) {
    m_rendered_dq_frames[i] = dq1[i].lerp(dq2[i], offset);
    m_rendered_dq_frames[i].real = Math::normalize(m_rendered_dq_frames[i].real);
  }

  if (completes) {
    if (_loop) {
      m_current_frame = 0.0f;
    } else {
      m_completed = true;
    }
  }
}

} // namespace Rx::Model
