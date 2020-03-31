#ifndef RX_MODEL_LOADER_H
#define RX_MODEL_LOADER_H
#include "rx/model/importer.h"

#include "rx/material/loader.h"

#include "rx/core/log.h"
#include "rx/core/json.h"

#include "rx/core/hints/empty_bases.h"

#include "rx/math/transform.h"

namespace rx::model {

struct RX_HINT_EMPTY_BASES loader
  : concepts::no_copy
  , concepts::no_move
{
  loader(memory::allocator& _allocator);
  ~loader();

  bool load(stream* _stream);
  bool load(const string& _file_name);

  bool parse(const json& _json);

  struct vertex {
    math::vec3f position;
    math::vec3f normal;
    math::vec4f tangent;
    math::vec2f coordinate;
  };

  struct animated_vertex {
    math::vec3f position;
    math::vec3f normal;
    math::vec4f tangent;
    math::vec2f coordinate;
    math::vec4b blend_weights;
    math::vec4b blend_indices;
  };

  bool is_animated() const;

  vector<vertex>&& vertices();
  vector<mesh>&& meshes();
  vector<rx_u32>&& elements();
  map<string, material::loader>&& materials();

  // Only valid for animated models.
  vector<animated_vertex>&& animated_vertices();
  vector<importer::joint>&& joints();
  const vector<importer::animation>& animations() const &;

  constexpr memory::allocator& allocator() const;

private:
  friend struct animation;

  template<typename... Ts>
  bool error(const char* _format, Ts&&... _arguments) const;

  template<typename... Ts>
  void log(log::level _level, const char* _format, Ts&&... _arguments) const;

  void write_log(log::level _level, string&& message_) const;

  bool import(const string& _file_name);
  bool parse_transform(const json& _transform);

  enum {
    k_constructed = 1 << 0,
    k_animated    = 1 << 1
  };

  memory::allocator& m_allocator;

  union {
    utility::nat as_nat;
    vector<vertex> as_vertices;
    vector<animated_vertex> as_animated_vertices;
  };

  vector<rx_u32> m_elements;
  vector<mesh> m_meshes;
  vector<importer::animation> m_animations;
  vector<importer::joint> m_joints;
  vector<math::vec3f> m_positions;
  vector<math::mat3x4f> m_frames;
  optional<math::transform> m_transform;
  map<string, material::loader> m_materials;
  string m_name;
  int m_flags;
};

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

inline bool loader::is_animated() const {
  return m_flags & k_animated;
}

inline vector<loader::vertex>&& loader::vertices() {
  RX_ASSERT(!is_animated(), "not a static model");
  return utility::move(as_vertices);
}

inline vector<mesh>&& loader::meshes() {
  return utility::move(m_meshes);
}

inline vector<rx_u32>&& loader::elements() {
  return utility::move(m_elements);
}

inline map<string, material::loader>&& loader::materials() {
  return utility::move(m_materials);
}

inline vector<loader::animated_vertex>&& loader::animated_vertices() {
  RX_ASSERT(is_animated(), "not a animated model");
  return utility::move(as_animated_vertices);
}

inline vector<importer::joint>&& loader::joints() {
  RX_ASSERT(is_animated(), "not a animated model");
  return utility::move(m_joints);
}

inline const vector<importer::animation>& loader::animations() const & {
  return m_animations;
}

RX_HINT_FORCE_INLINE constexpr memory::allocator& loader::allocator() const {
  return m_allocator;
}

} // namespace rx::model

#endif // RX_MODEL_LOADER_H
