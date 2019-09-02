#ifndef RX_MODEL_ANIMATION_H
#define RX_MODEL_ANIMATION_H
#include "rx/core/vector.h"

#include "rx/math/vec2.h"
#include "rx/math/mat3x4.h"
#include "rx/math/mat4x4.h"

namespace rx::render {
  struct immediate3D;
}

namespace rx::model {

struct interface;

struct animation {
  animation(interface* _model, rx_size _index);

  void update(rx_f32 _delta_time, bool _loop);

  const vector<math::mat3x4f>& frames() const &;
  rx_size joints() const;

  void render_skeleton(const math::mat4x4f& _world, render::immediate3D* _immediate);

private:
  interface* m_model;
  vector<math::mat3x4f> m_frames;
  rx_size m_animation;
  rx_f32 m_current_frame;
  bool m_completed;
};

inline const vector<math::mat3x4f>& animation::frames() const & {
  return m_frames;
}

} // namespace rx::model

#endif // RX_MODEL_ANIMATION