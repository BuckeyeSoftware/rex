#ifndef RX_DEMOS_MODEL_VIEWER_H
#define RX_DEMOS_MODEL_VIEWER_H
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

struct ModelViewer
  : Application
{
  ModelViewer(Engine* _engine);

  virtual bool on_init();
  virtual bool on_update(Float32 _delta_time);
  virtual bool on_render();
  virtual void on_resize(const Math::Vec2z& _resolution);

private:
  Optional<Render::GBuffer> m_gbuffer;
  Optional<Render::IndirectLightingPass> m_indirect_lighting_pass;
  Optional<Render::CopyPass> m_copy_pass;
  Optional<Render::ImageBasedLighting> m_image_based_lighting;
  Optional<Render::Model> m_model;
  Optional<Render::Skybox> m_skybox;
  Math::Camera m_camera;
  Math::Transform m_transform;
};

} // namespace Rx

#endif // RX_DEMOS_MODEL_VIEWER_H