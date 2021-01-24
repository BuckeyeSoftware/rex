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

inline IrradianceMap::~IrradianceMap() {
  release();
}

inline IrradianceMap::IrradianceMap(IrradianceMap&& move_)
  : m_frontend{Utility::exchange(move_.m_frontend, nullptr)}
  , m_environment_map{Utility::exchange(move_.m_environment_map, nullptr)}
  , m_target{Utility::exchange(move_.m_target, nullptr)}
  , m_texture{Utility::exchange(move_.m_texture, nullptr)}
  , m_technique{Utility::exchange(move_.m_technique, nullptr)}
  , m_resolution{Utility::exchange(move_.m_resolution, 0)}
  , m_current_face{Utility::exchange(move_.m_current_face, 0)}
{
}

inline IrradianceMap& IrradianceMap::operator=(IrradianceMap&& move_) {
  release();
  Utility::construct<IrradianceMap>(this, Utility::move(move_));
  return *this;
}

inline Frontend::TextureCM* IrradianceMap::texture() const {
  return m_texture;
}

struct PrefilteredEnvironmentMap {
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

inline PrefilteredEnvironmentMap::~PrefilteredEnvironmentMap() {
  release();
}

inline PrefilteredEnvironmentMap::PrefilteredEnvironmentMap(PrefilteredEnvironmentMap&& move_)
  : m_frontend{Utility::exchange(move_.m_frontend, nullptr)}
  , m_environment_map{Utility::exchange(move_.m_environment_map, nullptr)}
  , m_targets{Utility::exchange(move_.m_targets, {})}
  , m_texture{Utility::exchange(move_.m_texture, nullptr)}
  , m_technique{Utility::exchange(move_.m_technique, nullptr)}
  , m_resolution{Utility::exchange(move_.m_resolution, 0)}
  , m_current_face{Utility::exchange(move_.m_current_face, 0)}
{
}

inline PrefilteredEnvironmentMap& PrefilteredEnvironmentMap::operator=(PrefilteredEnvironmentMap&& move_) {
  release();
  Utility::construct<PrefilteredEnvironmentMap>(this, Utility::move(move_));
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
  void release();

  Frontend::Context* m_frontend;
  Frontend::Texture2D* m_scale_bias_texture;

  IrradianceMap m_irradiance_map;
  PrefilteredEnvironmentMap m_prefiltered_environment_map;
};

inline constexpr ImageBasedLighting::ImageBasedLighting()
  : m_frontend{nullptr}
  , m_scale_bias_texture{nullptr}
{
}

inline ImageBasedLighting::~ImageBasedLighting() {
  release();
}

inline ImageBasedLighting::ImageBasedLighting(ImageBasedLighting&& move_)
  : m_frontend{Utility::exchange(move_.m_frontend, nullptr)}
  , m_scale_bias_texture{Utility::exchange(move_.m_scale_bias_texture, nullptr)}
  , m_irradiance_map{Utility::move(move_.m_irradiance_map)}
  , m_prefiltered_environment_map{Utility::move(move_.m_prefiltered_environment_map)}
{
}

inline ImageBasedLighting& ImageBasedLighting::operator=(ImageBasedLighting&& move_) {
  release();
  Utility::construct<ImageBasedLighting>(this, Utility::move(move_));
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
