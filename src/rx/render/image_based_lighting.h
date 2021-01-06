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
  RX_MARK_NO_COPY(ImageBasedLighting);

  // ImageBasedLighting(Frontend::Context* _interface);
  constexpr ImageBasedLighting();
  ImageBasedLighting(ImageBasedLighting&& ibl_);
  ~ImageBasedLighting();
  ImageBasedLighting& operator=(ImageBasedLighting&& ibl_);

  static Optional<ImageBasedLighting> create(Frontend::Context* _frontend);

  void render(Frontend::TextureCM* _environment,
    Size _irradiance_map_size,
    ColorGrader::Entry* _grading = nullptr);

  Frontend::TextureCM* irradiance() const;
  Frontend::TextureCM* prefilter() const;
  Frontend::Texture2D* scale_bias() const;

private:
  void release();

  Frontend::Context* m_frontend;
  Frontend::TextureCM* m_irradiance_texture;
  Frontend::TextureCM* m_prefilter_texture;
  Frontend::Texture2D* m_scale_bias_texture;
};

inline constexpr ImageBasedLighting::ImageBasedLighting()
  : m_frontend{nullptr}
  , m_irradiance_texture{nullptr}
  , m_prefilter_texture{nullptr}
  , m_scale_bias_texture{nullptr}
{
}

inline ImageBasedLighting::~ImageBasedLighting() {
  release();
}

inline ImageBasedLighting& ImageBasedLighting::operator=(ImageBasedLighting&& ibl_) {
  release();
  Utility::construct<ImageBasedLighting>(this, Utility::move(ibl_));
  return *this;
}

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
