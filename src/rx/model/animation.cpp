#include "rx/render/immediate3D.h"

#include "rx/model/animation.h"
#include "rx/model/loader.h"

#include "rx/core/math/abs.h"
#include "rx/core/math/mod.h"

namespace rx::model {

animation::animation(loader* _model, rx_size _index)
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

  const rx_size joints{m_model->joints().size()};
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

void animation::render_skeleton(const math::mat4x4f& _world, render::immediate3D* _immediate) {
  const auto& joints{m_model->joints()};

  // submit spheres for the joints first, together so they get batched as one
  // draw call
  for (rx_size i{0}; i < joints.size(); i++) {
    const math::mat3x4f& frame{m_frames[i] * joints[i].frame};

    const math::vec3f& x{math::normalize({frame.x.x, frame.y.x, frame.z.x})};
    const math::vec3f& y{math::normalize({frame.x.y, frame.y.y, frame.z.y})};
    const math::vec3f& z{math::normalize({frame.x.z, frame.y.z, frame.z.z})};
    const math::vec3f& w{frame.x.w, frame.y.w, frame.z.w};

    const math::mat4x4f& joint{{x.x, x.y, x.z, 0.0f},
                               {y.x, y.y, y.z, 0.0f},
                               {z.x, z.y, z.z, 0.0f},
                               {w.x, w.y, w.z, 1.0f}};

    _immediate->frame_queue().record_solid_sphere(
      {16.0f, 16.0f},
      {0.5f, 0.5f, 1.0f, 1.0f},
      math::mat4x4f::scale({0.1f, 0.1f, 0.1f}) * joint * _world,
      0);
  }

  // now submit lines for the skeleton second, together so they get batched
  // as one draw call
  for (rx_size i{0}; i < joints.size(); i++) {
    const math::mat3x4f& frame{m_frames[i] * joints[i].frame};
    const math::vec3f& w{frame.x.w, frame.y.w, frame.z.w};
    const rx_s32 parent{joints[i].parent};

    if (parent >= 0) {
      const math::mat3x4f& parent_joint{joints[parent].frame};
      const math::mat3x4f& parent_frame{m_frames[parent] * parent_joint};
      const math::vec3f& parent_position{parent_frame.x.w, parent_frame.y.w, parent_frame.z.w};

      _immediate->frame_queue().record_line(
        math::mat4x4f::transform_point(w, _world),
        math::mat4x4f::transform_point(parent_position, _world),
        {0.5f, 0.5f, 1.0f, 1.0f},
        0);
    }
  }
}

} // namespace rx::model