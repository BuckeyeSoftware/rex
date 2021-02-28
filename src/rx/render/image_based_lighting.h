#ifndef RX_RENDER_IBL_H
#define RX_RENDER_IBL_H
#include "rx/core/optional.h"
#include "rx/core/array.h"
#include "rx/core/utility/exchange.h"

namespace Rx::Render {

namespace Frontend {
  struct Texture;
  struct Texture2D;
  struct TextureCM;
  struct Target;
  struct Program;

  struct Technique;

  struct Context;
} // namespace Frontend

struct IrradianceMap {
  RX_MARK_NO_COPY(IrradianceMap);

  constexpr IrradianceMap();
  ~IrradianceMap();
  IrradianceMap(IrradianceMap&&);
  IrradianceMap& operator=(IrradianceMap&&);

  static Optional<IrradianceMap> create(Frontend::Context* _frontend, Size _resolution);

  void render(Frontend::Texture* _env_map);

  Frontend::TextureCM* texture() const;

private:
  static void move(IrradianceMap* dst_, IrradianceMap* src_);

  void release();
  void render_next_face();

  Frontend::Context* m_frontend;
  Frontend::Texture* m_environment_map;
  Frontend::Target* m_target;
  Frontend::TextureCM* m_texture;
  Frontend::Technique* m_technique;
  Size m_resolution;
  Sint32 m_current_face;
};

// [IrradianceMap]
inline void IrradianceMap::move(IrradianceMap* dst_, IrradianceMap* src_) {
  dst_->m_frontend = Utility::exchange(src_->m_frontend, nullptr);
  dst_->m_environment_map = Utility::exchange(src_->m_environment_map, nullptr);
  dst_->m_target = Utility::exchange(src_->m_target, nullptr);
  dst_->m_texture = Utility::exchange(src_->m_texture, nullptr);
  dst_->m_technique = Utility::exchange(src_->m_technique, nullptr);
  dst_->m_resolution = Utility::exchange(src_->m_resolution, 0);
  dst_->m_current_face = Utility::exchange(src_->m_current_face, 0);
}

inline constexpr IrradianceMap::IrradianceMap()
  : m_frontend{nullptr}
  , m_environment_map{nullptr}
  , m_target{nullptr}
  , m_texture{nullptr}
  , m_technique{nullptr}
  , m_resolution{0}
  , m_current_face{0}
{
}

inline IrradianceMap::IrradianceMap(IrradianceMap&& move_) {
  move(this, &move_);
}

inline IrradianceMap::~IrradianceMap() {
  release();
}

inline IrradianceMap& IrradianceMap::operator=(IrradianceMap&& move_) {
  if (this != &move_) {
    release();
    move(this, &move_);
  }
  return *this;
}

inline Frontend::TextureCM* IrradianceMap::texture() const {
  return m_texture;
}

struct PrefilteredEnvironmentMap {
  // This includes the base level.
  static inline constexpr const auto MAX_PREFILTER_LEVELS = 6;

  RX_MARK_NO_COPY(PrefilteredEnvironmentMap);

  constexpr PrefilteredEnvironmentMap();
  ~PrefilteredEnvironmentMap();
  PrefilteredEnvironmentMap(PrefilteredEnvironmentMap&&);
  PrefilteredEnvironmentMap& operator=(PrefilteredEnvironmentMap&&);

  static Optional<PrefilteredEnvironmentMap> create(Frontend::Context* _frontend, Size _resolution);

  void render(Frontend::Texture* _env_map);

  Frontend::TextureCM* texture() const;

private:
  static void move(PrefilteredEnvironmentMap* dst_, PrefilteredEnvironmentMap* src_);

  void release();
  void render_next_face();

  Frontend::Context* m_frontend;
  Frontend::Texture* m_environment_map;
  Array<Frontend::Target*[MAX_PREFILTER_LEVELS]> m_targets;
  Frontend::TextureCM* m_texture;
  Frontend::Technique* m_technique;
  Size m_resolution;
  Sint32 m_current_face;
};

// [PrefilteredEnvironmentMap]
inline void PrefilteredEnvironmentMap::move(PrefilteredEnvironmentMap* dst_, PrefilteredEnvironmentMap* src_) {
  dst_->m_frontend = Utility::exchange(src_->m_frontend, nullptr);
  dst_->m_environment_map = Utility::exchange(src_->m_environment_map, nullptr);
  dst_->m_targets = Utility::exchange(src_->m_targets, {});
  dst_->m_texture = Utility::exchange(src_->m_texture, nullptr);
  dst_->m_technique = Utility::exchange(src_->m_technique, nullptr);
  dst_->m_resolution = Utility::exchange(src_->m_resolution, 0);
  dst_->m_current_face = Utility::exchange(src_->m_current_face, 0);
}

inline constexpr PrefilteredEnvironmentMap::PrefilteredEnvironmentMap()
  : m_frontend{nullptr}
  , m_environment_map{nullptr}
  , m_targets{}
  , m_texture{nullptr}
  , m_technique{nullptr}
  , m_resolution{0}
  , m_current_face{0}
{
}

inline PrefilteredEnvironmentMap::PrefilteredEnvironmentMap(PrefilteredEnvironmentMap&& move_) {
  move(this, &move_);
}

inline PrefilteredEnvironmentMap::~PrefilteredEnvironmentMap() {
  release();
}

inline PrefilteredEnvironmentMap& PrefilteredEnvironmentMap::operator=(PrefilteredEnvironmentMap&& move_) {
  if (this != &move_) {
    release();
    move(this, &move_);
  }
  return *this;
}

inline Frontend::TextureCM* PrefilteredEnvironmentMap::texture() const {
  return m_texture;
}

struct ImageBasedLighting {
  RX_MARK_NO_COPY(ImageBasedLighting);

  constexpr ImageBasedLighting();
  ~ImageBasedLighting();
  ImageBasedLighting(ImageBasedLighting&&);
  ImageBasedLighting& operator=(ImageBasedLighting&&);

  static Optional<ImageBasedLighting> create(Frontend::Context* _frontend,
    Size _irradiance_size = 32, Size _prefilter_size = 256);

  void render(Frontend::Texture* _env_map);

  Frontend::Texture2D* scale_bias() const;
  Frontend::TextureCM* irradiance_map() const;
  Frontend::TextureCM* prefilter() const;

private:
  static void move(ImageBasedLighting* dst_, ImageBasedLighting* src_);

  void release();

  Frontend::Context* m_frontend;
  Frontend::Texture2D* m_scale_bias_texture;

  IrradianceMap m_irradiance_map;
  PrefilteredEnvironmentMap m_prefiltered_environment_map;
};

// [ImageBasedLighting]
inline void ImageBasedLighting::move(ImageBasedLighting* dst_, ImageBasedLighting* src_) {
  dst_->m_frontend = Utility::exchange(src_->m_frontend, nullptr);
  dst_->m_scale_bias_texture = Utility::exchange(src_->m_scale_bias_texture, nullptr);
  dst_->m_irradiance_map = Utility::move(src_->m_irradiance_map);
  dst_->m_prefiltered_environment_map = Utility::move(src_->m_prefiltered_environment_map);
}

inline constexpr ImageBasedLighting::ImageBasedLighting()
  : m_frontend{nullptr}
  , m_scale_bias_texture{nullptr}
{
}

inline ImageBasedLighting::ImageBasedLighting(ImageBasedLighting&& move_) {
  move(this, &move_);
}

inline ImageBasedLighting::~ImageBasedLighting() {
  release();
}

inline ImageBasedLighting& ImageBasedLighting::operator=(ImageBasedLighting&& move_) {
  if (this != &move_) {
    release();
    move(this, &move_);
  }
  return *this;
}

inline Frontend::Texture2D* ImageBasedLighting::scale_bias() const {
  return m_scale_bias_texture;
}

inline Frontend::TextureCM* ImageBasedLighting::irradiance_map() const {
  return m_irradiance_map.texture();
}

inline Frontend::TextureCM* ImageBasedLighting::prefilter() const {
  return m_prefiltered_environment_map.texture();
}

} // namespace Rx::Render

#endif // RX_RENDER_IBL_H
