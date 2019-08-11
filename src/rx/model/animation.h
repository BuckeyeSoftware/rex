#ifndef RX_MODEL_ANIMATION_H
#define RX_MODEL_ANIMATION_H
#include "rx/core/array.h"

#include "rx/math/vec2.h"
#include "rx/math/mat3x4.h"

namespace rx::model {

struct interface;

struct animation {
  animation(interface* _model, rx_size _index);

  void update(rx_f32 _delta_time, bool _loop);

  const array<math::mat3x4f>& frames() const &;
  rx_size joints() const;

private:
  interface* m_model;
  array<math::mat3x4f> m_frames;
  rx_size m_animation;
  rx_f32 m_current_frame;
  bool m_completed;
};

inline const array<math::mat3x4f>& animation::frames() const & {
  return m_frames;
}

} // namespace rx::model

#endif // RX_MODEL_ANIMATION