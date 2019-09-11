#ifndef RX_RENDER_FRONTEND_MATERIAL_H
#define RX_RENDER_FRONTEND_MATERIAL_H
#include "rx/core/string.h"
#include "rx/core/concepts/no_copy.h"

#include "rx/material/loader.h"

namespace rx::render::frontend {

struct texture2D;
struct interface;

struct material
  : concepts::no_copy
{
  material(interface* _frontend);
  ~material();

  material(material&& _material);
  material& operator=(material&& _material);

  bool load(rx::material::loader&& _loader);

  bool alpha_test() const;
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
  string m_name;
};

inline material::material(material&& _material)
  : m_frontend{_material.m_frontend}
  , m_diffuse{_material.m_diffuse}
  , m_normal{_material.m_normal}
  , m_metalness{_material.m_metalness}
  , m_roughness{_material.m_roughness}
  , m_alpha_test{_material.m_alpha_test}
  , m_name{utility::move(_material.m_name)}
{
  _material.m_diffuse = nullptr;
  _material.m_normal = nullptr;
  _material.m_metalness = nullptr;
  _material.m_roughness = nullptr;
  _material.m_alpha_test = false;
}

inline material& material::operator=(material&& _material) {
  this->~material();

  m_frontend = _material.m_frontend;
  m_diffuse = _material.m_diffuse;
  m_normal = _material.m_normal;
  m_metalness = _material.m_metalness;
  m_roughness = _material.m_roughness;
  m_alpha_test = _material.m_alpha_test;
  m_name = utility::move(_material.m_name);

  _material.m_diffuse = nullptr;
  _material.m_normal = nullptr;
  _material.m_metalness = nullptr;
  _material.m_roughness = nullptr;
  _material.m_alpha_test = false;

  return *this;
}

inline const string& material::name() const & {
  return m_name;
}

inline bool material::alpha_test() const {
  return m_alpha_test;
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