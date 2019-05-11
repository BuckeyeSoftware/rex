#ifndef RX_RENDER_MATERIAL_H
#define RX_RENDER_MATERIAL_H

#include <rx/core/string.h>
#include <rx/core/json.h>
#include <rx/core/log.h>

namespace rx::render {

struct texture2D;
struct frontend;

struct material {
  material(frontend* _frontend);
  ~material();

  bool load(const string& _file_name);
  const string& name() const &;

  texture2D* diffuse() const;
  texture2D* normal() const;
  texture2D* metal() const;
  texture2D* roughness() const;

private:
  bool parse(const json& _data);
  bool parse_texture(const json& _texture);
  bool parse_wrap(texture2D* _texture, const json& _wrap);
  bool parse_filter(texture2D* _texture, const json& _filter, bool _mipmaps);

  template<typename... Ts>
  bool error(const char* _format, Ts&&... _arguments) const;

  template<typename... Ts>
  void log(log::level _level, const char* _format, Ts&&... _arguments) const;

  void write_log(log::level _level, string&& _message) const;

  frontend* m_frontend;

  texture2D* m_diffuse;
  texture2D* m_normal;
  texture2D* m_metal;
  texture2D* m_roughness;

  string m_name;
};

inline texture2D* material::diffuse() const {
  return m_diffuse;
}

inline texture2D* material::normal() const {
  return m_normal;
}

inline texture2D* material::metal() const {
  return m_metal;
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

} // namespace rx::render

#endif // RX_RENDER_MATERIAL_H