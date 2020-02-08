#ifndef RX_MATERIAL_LOADER_H
#define RX_MATERIAL_LOADER_H
#include "rx/core/vector.h"
#include "rx/core/string.h"
#include "rx/core/map.h"
#include "rx/core/log.h"

#include "rx/math/transform.h"

#include "rx/material/texture.h"

namespace rx {
  struct json;
  struct stream;
}

namespace rx::material {

struct loader
  : concepts::no_copy
{
  loader(memory::allocator* _allocator);
  loader(loader&& loader_);

  void operator=(loader&& loader_);

  bool load(stream* _stream);
  bool load(const string& _file_name);

  bool parse(const json& _definition);

  memory::allocator* allocator() const;
  vector<texture>&& textures();
  string&& name();
  bool alpha_test() const;
  bool has_alpha() const;
  bool no_compress() const;
  rx_f32 roughness() const;
  rx_f32 metalness() const;
  const optional<math::transform>& transform() const &;

private:
  template<typename... Ts>
  bool error(const char* _format, Ts&&... _arguments) const;

  template<typename... Ts>
  void log(log::level _level, const char* _format, Ts&&... _arguments) const;

  void write_log(log::level _level, string&& message_) const;

  bool parse_textures(const json& _textures);

  enum {
    k_alpha_test  = 1 << 0,
    k_has_alpha   = 1 << 1,
    k_no_compress = 1 << 2
  };

  memory::allocator* m_allocator;
  vector<texture> m_textures;
  string m_name;
  rx_u32 m_flags;
  rx_f32 m_roughness;
  rx_f32 m_metalness;
  optional<math::transform> m_transform;
};

inline memory::allocator* loader::allocator() const {
  return m_allocator;
}

inline vector<texture>&& loader::textures() {
  return utility::move(m_textures);
}

inline string&& loader::name() {
  return utility::move(m_name);
}

inline bool loader::alpha_test() const {
  return m_flags & k_alpha_test;
}

inline bool loader::has_alpha() const {
  return m_flags & k_has_alpha;
}

inline bool loader::no_compress() const {
  return m_flags & k_no_compress;
}

inline rx_f32 loader::roughness() const {
  return m_roughness;
}

inline rx_f32 loader::metalness() const {
  return m_metalness;
}

inline const optional<math::transform>& loader::transform() const & {
  return m_transform;
}

template<typename... Ts>
inline bool loader::error(const char* _format, Ts&&... _arguments) const {
  log(log::level::k_error, _format, utility::forward<Ts>(_arguments)...);
  return false;
}

template<typename... Ts>
inline void loader::log(log::level _level, const char* _format,
  Ts&&... _arguments) const
{
  write_log(_level, string::format(_format, utility::forward<Ts>(_arguments)...));
}

} // namespace rx::material

#endif // RX_MATERIAL_LOADER_H
