#include "rx/render/immediate3D.h"

#include "rx/model/animation.h"
#include "rx/model/loader.h"

#include "rx/core/math/abs.h"
#include "rx/core/math/mod.h"

namespace Rx::Model {

Animation::Animation(const Loader* _model, Size _index)
  : m_model{_model}
  , m_frames{m_model->m_frames}
  , m_animation{_index}
  , m_current_frame{0.0f}
  , m_completed{false}
{
}

Size Animation::joints() const {
  return m_model->joints().size();
}

void Animation::update(Float32 _delta_time, bool _loop) {
  if (m_completed) {
    return;
  }

  const auto& animation{m_model->m_animations[m_animation]};

  m_current_frame += animation.frame_rate * _delta_time;

  const bool completes{m_current_frame >= animation.frame_count - 1};
  const bool finished{completes && !_loop};
  m_current_frame = Math::mod(m_current_frame,
                              static_cast<Float32>(animation.frame_count));

  Size frame1{finished ? animation.frame_count - 1 : static_cast<Size>(m_current_frame)};
  Size frame2{finished ? animation.frame_count - 1 : frame1 + 1};

  frame1 %= animation.frame_count;
  frame2 %= animation.frame_count;

  Math::Vec2z frame_indices;
  frame_indices[0] = animation.frame_offset + frame1;
  frame_indices[1] = animation.frame_offset + frame2;

  const Float32 offset =
          finished ? 0.0f : Math::abs(m_current_frame - frame1);

  m_interpolant.frame1 = frame1;
  m_interpolant.frame2 = frame2;
  m_interpolant.offset = offset;

  const Size joints{m_model->joints().size()};
  const Math::Mat3x4f* mat1{&m_model->m_frames[frame_indices[0] * joints]};
  const Math::Mat3x4f* mat2{&m_model->m_frames[frame_indices[1] * joints]};

  // Interpolate matrices between the two closest frames.
  for (Size i = 0; i < joints; i++) {
    m_frames[i] = mat1[i] * (1.0f - offset) + mat2[i] * offset;
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
