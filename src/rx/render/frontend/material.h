#ifndef RX_RENDER_FRONTEND_MATERIAL_H
#define RX_RENDER_FRONTEND_MATERIAL_H
#include "rx/core/string.h"

#include "rx/core/concepts/no_copy.h"

#include "rx/core/hints/empty_bases.h"

#include "rx/math/transform.h"

#include "rx/material/loader.h"

namespace Rx::Render::Frontend {

struct Texture2D;
struct Context;

struct RX_HINT_EMPTY_BASES Material
  : Concepts::NoCopy
{
  Material(Context* _frontend);
  ~Material();

  Material(Material&& material_);
  Material& operator=(Material&& material_);

  bool load(Rx::Material::Loader&& loader_);

  bool alpha_test() const;
  bool has_alpha() const;
  const Optional<Math::Transform>& transform() const &;
  const String& name() const &;

  Texture2D* albedo() const;
  Texture2D* normal() const;
  Texture2D* roughness() const;
  Texture2D* metalness() const;
  Texture2D* ambient() const;
  Texture2D* emissive() const;

  Float32 roughness_value() const;
  Float32 metalness_value() const;

private:
  Context* m_frontend;
  Texture2D* m_albedo;
  Texture2D* m_normal;
  Texture2D* m_roughness;
  Texture2D* m_metalness;
  Texture2D* m_ambient;
  Texture2D* m_emissive;
  bool m_alpha_test;
  bool m_has_alpha;
  Float32 m_roughness_value;
  Float32 m_metalness_value;
  String m_name;
  Optional<Math::Transform> m_transform;
};

inline Material::Material(Material&& material_)
  : m_frontend{material_.m_frontend}
  , m_albedo{Utility::exchange(material_.m_albedo, nullptr)}
  , m_normal{Utility::exchange(material_.m_normal, nullptr)}
  , m_roughness{Utility::exchange(material_.m_roughness, nullptr)}
  , m_metalness{Utility::exchange(material_.m_metalness, nullptr)}
  , m_ambient{Utility::exchange(material_.m_ambient, nullptr)}
  , m_emissive{Utility::exchange(material_.m_emissive, nullptr)}
  , m_alpha_test{Utility::exchange(material_.m_alpha_test, false)}
  , m_has_alpha{Utility::exchange(material_.m_has_alpha, false)}
  , m_roughness_value{Utility::exchange(material_.m_roughness_value, 1.0f)}
  , m_metalness_value{Utility::exchange(material_.m_metalness_value, 0.0f)}
  , m_name{Utility::move(material_.m_name)}
  , m_transform{Utility::move(material_.m_transform)}
{
}

inline Material& Material::operator=(Material&& material_) {
  RX_ASSERT(&material_ != this, "self assignment");

  this->~Material();

  m_frontend = material_.m_frontend;
  m_albedo = Utility::exchange(material_.m_albedo, nullptr);
  m_normal = Utility::exchange(material_.m_normal, nullptr);
  m_roughness = Utility::exchange(material_.m_roughness, nullptr);
  m_metalness = Utility::exchange(material_.m_metalness, nullptr);
  m_ambient = Utility::exchange(material_.m_ambient, nullptr);
  m_emissive = Utility::exchange(material_.m_emissive, nullptr);
  m_alpha_test = Utility::exchange(material_.m_alpha_test, false);
  m_has_alpha = Utility::exchange(material_.m_has_alpha, false);
  m_roughness_value = Utility::exchange(material_.m_roughness_value, 1.0f);
  m_metalness_value = Utility::exchange(material_.m_metalness_value, 0.0f);
  m_name = Utility::move(material_.m_name);
  m_transform = Utility::move(material_.m_transform);

  return *this;
}

inline const String& Material::name() const & {
  return m_name;
}

inline bool Material::alpha_test() const {
  return m_alpha_test;
}

inline bool Material::has_alpha() const {
  return m_has_alpha;
}

inline const Optional<Math::Transform>& Material::transform() const & {
  return m_transform;
}

inline Texture2D* Material::albedo() const {
  return m_albedo;
}

inline Texture2D* Material::normal() const {
  return m_normal;
}

inline Texture2D* Material::roughness() const {
  return m_roughness;
}

inline Texture2D* Material::metalness() const {
  return m_metalness;
}

inline Texture2D* Material::ambient() const {
  return m_ambient;
}

inline Texture2D* Material::emissive() const {
  return m_emissive;
}

inline Float32 Material::roughness_value() const {
  return m_roughness_value;
}

inline Float32 Material::metalness_value() const {
  return m_metalness_value;
}

} // namespace rx::render::frontend

#endif // RX_RENDER_FRONTEND_MATERIAL_H
