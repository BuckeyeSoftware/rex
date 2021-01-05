#ifndef RX_RENDER_IBL_H
#define RX_RENDER_IBL_H
#include "rx/core/array.h"
#include "rx/render/color_grader.h"

namespace Rx::Render {

namespace Frontend {
  struct Texture2D;
  struct TextureCM;

  struct Technique;
  struct Context;
} // namespace frontend

struct ImageBasedLighting {
  ImageBasedLighting(Frontend::Context* _interface);
  ~ImageBasedLighting();

  void render(Frontend::TextureCM* _environment, Size _irradiance_map_size, ColorGrader::Entry* _grading = nullptr);

  Frontend::TextureCM* irradiance() const;
  Frontend::TextureCM* prefilter() const;
  Frontend::Texture2D* scale_bias() const;

private:
  Frontend::Context* m_frontend;
  Frontend::TextureCM* m_irradiance_texture;
  Frontend::TextureCM* m_prefilter_texture;
  Frontend::Texture2D* m_scale_bias_texture;
};

inline Frontend::TextureCM* ImageBasedLighting::irradiance() const {
  return m_irradiance_texture;
}

inline Frontend::TextureCM* ImageBasedLighting::prefilter() const {
  return m_prefilter_texture;
}

inline Frontend::Texture2D* ImageBasedLighting::scale_bias() const {
  return m_scale_bias_texture;
}

} // namespace Rx::Render

#endif // RX_RENDER_IBL_H
