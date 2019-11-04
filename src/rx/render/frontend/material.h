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

  texture2D* diffuse() const;
  texture2D* normal() const;
  texture2D* metalness() const;
  texture2D* roughness() const;

private:
  interface* m_frontend;
  texture2D* m_diffuse;
  texture2D* m_normal;
  texture2D* m_metalness;
  texture2D* m_roughness;
  bool m_alpha_test;
  bool m_has_alpha;
  string m_name;
  optional<math::transform> m_transform;
};

inline material::material(material&& material_)
  : m_frontend{material_.m_frontend}
  , m_diffuse{material_.m_diffuse}
  , m_normal{material_.m_normal}
  , m_metalness{material_.m_metalness}
  , m_roughness{material_.m_roughness}
  , m_alpha_test{material_.m_alpha_test}
  , m_has_alpha{material_.m_has_alpha}
  , m_name{utility::move(material_.m_name)}
  , m_transform{utility::move(material_.m_transform)}
{
  material_.m_diffuse = nullptr;
  material_.m_normal = nullptr;
  material_.m_metalness = nullptr;
  material_.m_roughness = nullptr;
  material_.m_alpha_test = false;
  material_.m_has_alpha = false;
}

inline material& material::operator=(material&& material_) {
  RX_ASSERT(&material_ != this, "self assignment");

  this->~material();

  m_frontend = material_.m_frontend;
  m_diffuse = material_.m_diffuse;
  m_normal = material_.m_normal;
  m_metalness = material_.m_metalness;
  m_roughness = material_.m_roughness;
  m_alpha_test = material_.m_alpha_test;
  m_has_alpha = material_.m_has_alpha;
  m_name = utility::move(material_.m_name);
  m_transform = utility::move(material_.m_transform);

  material_.m_diffuse = nullptr;
  material_.m_normal = nullptr;
  material_.m_metalness = nullptr;
  material_.m_roughness = nullptr;
  material_.m_alpha_test = false;
  material_.m_has_alpha = false;

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

inline texture2D* material::diffuse() const {
  return m_diffuse;
}

inline texture2D* material::normal() const {
  return m_normal;
}

inline texture2D* material::metalness() const {
  return m_metalness;
}

inline texture2D* material::roughness() const {
  return m_roughness;
}

} // namespace rx::render::frontend

#endif // RX_RENDER_FRONTEND_MATERIAL_H
