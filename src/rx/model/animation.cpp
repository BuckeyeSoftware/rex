#include "rx/model/animation.h"
#include "rx/model/interface.h"

#include "rx/core/math/abs.h"
#include "rx/core/math/mod.h"

namespace rx::model {

animation::animation(interface* _model, rx_size _index)
  : m_model{_model}
  , m_animation{_index}
{
  m_frames = _model->m_frames;
}

void animation::update(rx_f32 _delta_time, bool _loop) {
  if (m_completed) {
    return;
  }

  const auto& animation{m_model->m_animations[m_animation]};

  m_current_frame += animation.frame_rate * _delta_time;

  const bool completes{m_current_frame >= animation.frame_count - 1};
  const bool finished{completes && !_loop};
  m_current_frame = math::mod(m_current_frame,
    static_cast<rx_f32>(animation.frame_count));

  rx_size frame1{finished ? animation.frame_count - 1 : static_cast<rx_size>(m_current_frame)};
  rx_size frame2{finished ? animation.frame_count - 1 : frame1 + 1};

  frame1 %= animation.frame_count;
  frame2 %= animation.frame_count;

  math::vec2z frame_indices;
  frame_indices[0] = animation.frame_offset + frame1;
  frame_indices[1] = animation.frame_offset + frame2;

  const rx_f32 offset{finished ? 0.0f : math::abs(m_current_frame - frame1)};

  const rx_size joints{m_model->m_joints};
  const math::mat3x4f* mat1{&m_model->m_frames[frame_indices[0] * joints]};
  const math::mat3x4f* mat2{&m_model->m_frames[frame_indices[1] * joints]};

  // interpolate matrices between the two closest frames
  for (rx_size i{0}; i < joints; i++) {
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

} // namespace rx::model