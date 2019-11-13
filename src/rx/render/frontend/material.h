#ifndef RX_RENDER_FRONTEND_MATERIAL_H
#define RX_RENDER_FRONTEND_MATERIAL_H
#include "rx/core/string.h"
#include "rx/core/concepts/no_copy.h"

#include "rx/math/transform.h"

#include "rx/material/loader.h"

namespace rx::render::frontend {

struct texture2D;
struct interface;

struct material
  : concepts::no_copy
{
  material(interface* _frontend);
  ~material();

  material(material&& material_);
  material& operator=(material&& material_);

  bool load(rx::material::loader&& loader_);

  bool alpha_test() const;
  bool has_alpha() const;
  const optional<math::transform>& transform() const &;
  const string& name() const &;

  texture2D* albedo() const;
  texture2D* normal() const;
  texture2D* roughness() const;
  texture2D* metalness() const;
  texture2D* ambient() const;
  texture2D* emissive() const;

  rx_f32 roughness_value() const;
  rx_f32 metalness_value() const;

private:
  interface* m_frontend;
  texture2D* m_albedo;
  texture2D* m_normal;
  texture2D* m_roughness;
  texture2D* m_metalness;
  texture2D* m_ambient;
  texture2D* m_emissive;
  bool m_alpha_test;
  bool m_has_alpha;
  rx_f32 m_roughness_value;
  rx_f32 m_metalness_value;
  string m_name;
  optional<math::transform> m_transform;
};

inline material::material(material&& material_)
  : m_frontend{material_.m_frontend}
  , m_albedo{material_.m_albedo}
  , m_normal{material_.m_normal}
  , m_roughness{material_.m_roughness}
  , m_metalness{material_.m_metalness}
  , m_ambient{material_.m_ambient}
  , m_emissive{material_.m_emissive}
  , m_alpha_test{material_.m_alpha_test}
  , m_has_alpha{material_.m_has_alpha}
  , m_roughness_value{material_.m_roughness_value}
  , m_metalness_value{material_.m_metalness_value}
  , m_name{utility::move(material_.m_name)}
  , m_transform{utility::move(material_.m_transform)}
{
  material_.m_albedo = nullptr;
  material_.m_normal = nullptr;
  material_.m_metalness = nullptr;
  material_.m_roughness = nullptr;
  material_.m_ambient = nullptr;
  material_.m_emissive = nullptr;
  material_.m_alpha_test = false;
  material_.m_has_alpha = false;
  material_.m_roughness_value = 1.0f;
  material_.m_metalness_value = 0.0f;
}

inline material& material::operator=(material&& material_) {
  RX_ASSERT(&material_ != this, "self assignment");

  this->~material();

  m_frontend = material_.m_frontend;
  m_albedo = material_.m_albedo;
  m_normal = material_.m_normal;
  m_metalness = material_.m_metalness;
  m_roughness = material_.m_roughness;
  m_ambient = material_.m_ambient;
  m_emissive = material_.m_emissive;
  m_alpha_test = material_.m_alpha_test;
  m_has_alpha = material_.m_has_alpha;
  m_roughness_value = material_.m_roughness_value;
  m_metalness_value = material_.m_metalness_value;
  m_name = utility::move(material_.m_name);
  m_transform = utility::move(material_.m_transform);

  material_.m_albedo = nullptr;
  material_.m_normal = nullptr;
  material_.m_metalness = nullptr;
  material_.m_roughness = nullptr;
  material_.m_ambient = nullptr;
  material_.m_emissive = nullptr;
  material_.m_alpha_test = false;
  material_.m_has_alpha = false;
  material_.m_roughness_value = 1.0f;
  material_.m_metalness_value = 0.0f;

  return *this;
}

inline const string& material::name() const & {
  return m_name;
}

inline bool material::alpha_test() const {
  return m_alpha_test;
}

inline bool material::has_alpha() const {
  return m_has_alpha;
}

inline const optional<math::transform>& material::transform() const & {
  return m_transform;
}

inline texture2D* material::albedo() const {
  return m_albedo;
}

inline texture2D* material::normal() const {
  return m_normal;
}

inline texture2D* material::roughness() const {
  return m_roughness;
}

inline texture2D* material::metalness() const {
  return m_metalness;
}

inline texture2D* material::ambient() const {
  return m_ambient;
}

inline texture2D* material::emissive() const {
  return m_emissive;
}

inline rx_f32 material::roughness_value() const {
  return m_roughness_value;
}

inline rx_f32 material::metalness_value() const {
  return m_metalness_value;
}

} // namespace rx::render::frontend

#endif // RX_RENDER_FRONTEND_MATERIAL_H
