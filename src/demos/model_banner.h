#ifndef RX_DEMOS_MODEL_BANNER_H
#define RX_DEMOS_MODEL_BANNER_H

#include "rx/application.h"

#include "rx/render/model.h"
#include "rx/render/skybox.h"
#include "rx/render/gbuffer.h"
#include "rx/render/image_based_lighting.h"
#include "rx/render/indirect_lighting_pass.h"
#include "rx/render/copy_pass.h"

#include "rx/math/camera.h"
#include "rx/math/vec2.h"

namespace Rx {

inline auto sampler(Float32 _time, const auto& _start, const auto& _end, Float32 _duration) {
  const auto change = _end - _start;
  _time /= _duration;
  return (change * -1.0f) * _time * (_time - 2.0f) + _start;
}

template<typename T>
struct Animation {
  Animation(T& property_, const T& _end, Float32 _duration)
    : m_current{&property_}
    , m_start{property_}
    , m_end{_end}
    , m_current_time{0.0f}
    , m_end_time{_duration}
  {
  }

  void update(Float32 _delta_time) {
    if (m_current_time >= m_end_time) {
      return;
    }

    *m_current = sampler(m_current_time, m_start, m_end, m_end_time);
    m_current_time += _delta_time;

    if (m_current_time >= m_end_time) {
      *m_current = m_end;
      m_current_time = m_end_time;
    }
  }

private:
  T *m_current;
  T m_start;
  T m_end;
  Float32 m_current_time;
  Float32 m_end_time;
};

struct ModelBanner
  : Application
{
  ModelBanner(Engine* _engine);

  virtual bool on_init();
  virtual bool on_update(Float32 _delta_time);
  virtual bool on_render();
  virtual void on_resize(const Math::Vec2z& _resolution);

private:
  Optional<Render::GBuffer> m_gbuffer;
  Optional<Render::IndirectLightingPass> m_indirect_lighting_pass;
  Optional<Render::CopyPass> m_copy_pass;
  Optional<Render::ImageBasedLighting> m_image_based_lighting;
  Vector<Render::Model> m_models;
  Optional<Render::Skybox> m_skybox;
  Vector<Math::Transform> m_transforms;
  Math::Camera m_camera;
  Size m_selected;
  Optional<Animation<Float32>> m_animation;
};
} // namespace Rx

#endif // RX_DEMOS_MODEL_BANNER_H