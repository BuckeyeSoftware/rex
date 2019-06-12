#ifndef RX_RENDER_FRONTEND_MATERIAL_H
#define RX_RENDER_FRONTEND_MATERIAL_H

#include "rx/core/string.h"
#include "rx/core/json.h"
#include "rx/core/log.h"

#include "rx/core/concepts/no_copy.h"

namespace rx::render::frontend {

struct texture2D;
struct interface;

struct material : concepts::no_copy {
  material(interface* _frontend);
  ~material();

  material(material&& _material);
  material& operator=(material&& _material);

  bool load(const string& _file_name);
  const string& name() const &;

  texture2D* diffuse() const;
  texture2D* normal() const;
  texture2D* metalness() const;
  texture2D* roughness() const;

private:
  void fini();

  bool parse(const json& _data);
  bool parse_texture(const json& _texture);
  bool parse_wrap(texture2D* _texture, const json& _wrap);
  bool parse_filter(texture2D* _texture, const json& _filter, bool _mipmaps);

  template<typename... Ts>
  bool error(const char* _format, Ts&&... _arguments) const;

  template<typename... Ts>
  void log(log::level _level, const char* _format, Ts&&... _arguments) const;

  void write_log(log::level _level, string&& _message) const;

  interface* m_frontend;

  texture2D* m_diffuse;
  texture2D* m_normal;
  texture2D* m_metalness;
  texture2D* m_roughness;

  string m_name;
};

inline material::material(material&& _material)
  : m_frontend{_material.m_frontend}
  , m_diffuse{_material.m_diffuse}
  , m_normal{_material.m_normal}
  , m_metalness{_material.m_metalness}
  , m_roughness{_material.m_roughness}
  , m_name{utility::move(_material.m_name)}
{
  _material.m_diffuse = nullptr;
  _material.m_normal = nullptr;
  _material.m_metalness = nullptr;
  _material.m_roughness = nullptr;
}

inline material& material::operator=(material&& _material) {
  fini();

  m_frontend = _material.m_frontend;
  m_diffuse = _material.m_diffuse;
  m_normal = _material.m_normal;
  m_metalness = _material.m_metalness;
  m_roughness = _material.m_roughness;
  m_name = utility::move(_material.m_name);

  _material.m_diffuse = nullptr;
  _material.m_normal = nullptr;
  _material.m_metalness = nullptr;
  _material.m_roughness = nullptr;

  return *this;
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

template<typename... Ts>
inline bool material::error(const char* _format, Ts&&... _arguments) const {
  log(log::level::k_error, "%s", string::format(_format, utility::forward<Ts>(_arguments)...));
  return false;
}

template<typename... Ts>
inline void material::log(log::level _level, const char* _format,
  Ts&&... _arguments) const
{
  write_log(_level, string::format(_format, utility::forward<Ts>(_arguments)...));
}

} // namespace rx::render::frontend

#endif // RX_RENDER_FRONTEND_MATERIAL_H