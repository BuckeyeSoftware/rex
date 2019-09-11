#ifndef RX_MODEL_LOADER_H
#define RX_MODEL_LOADER_H
#include "rx/model/importer.h"

#include "rx/core/log.h"
#include "rx/core/json.h"

namespace rx::model {

struct loader {
  loader(memory::allocator* _allocator);
  ~loader();

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

  const vector<vertex>& vertices() const &;
  vector<vertex>&& vertices() &&;
  const vector<animated_vertex> animated_vertices() const &;
  vector<animated_vertex>&& animated_vertices() &&;
  const vector<mesh>& meshes() const &;
  const vector<rx_u32>& elements() const &;
  vector<rx_u32>&& elements() &&;
  const vector<importer::joint>& joints() const &;
  const json& materials() const &;

private:
  friend struct animation;

  template<typename... Ts>
  bool error(const char* _format, Ts&&... _arguments) const;

  template<typename... Ts>
  void log(log::level _level, const char* _format, Ts&&... _arguments) const;

  void write_log(log::level _level, string&& _message) const;

  bool import(const string& _file_name);

  enum {
    k_constructed = 1 << 0,
    k_animated    = 1 << 1
  };

  memory::allocator* m_allocator;

  union {
    utility::nat as_nat;
    vector<vertex> as_vertices;
    vector<animated_vertex> as_animated_vertices;
  };

  vector<rx_u32> m_elements;
  vector<mesh> m_meshes;
  vector<importer::animation> m_animations;
  vector<importer::joint> m_joints;
  vector<math::mat3x4f> m_frames;
  string m_name;

  // TODO(dweiler):
  // The following JSON object exists for |load| to populate to allow
  // |m_materials| to exist potentially long after the contents are loaded
  // since m_materials shares memory of the root JSON object.
  //
  // We need to implement some sort of JSON clone to support this correctly.
  json m_definition;
  
  json m_materials;
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

inline loader::loader(memory::allocator* _allocator)
  : m_allocator{_allocator}
  , as_nat{}
  , m_elements{m_allocator}
  , m_meshes{m_allocator}
  , m_joints{m_allocator}
  , m_frames{m_allocator}
  , m_flags{0}
{
}

inline loader::~loader() {
  if (m_flags & k_constructed) {
    if (m_flags & k_animated) {
      utility::destruct<vector<animated_vertex>>(&as_animated_vertices);
    } else {
      utility::destruct<vector<vertex>>(&as_vertices);
    }
  }
}

inline bool loader::is_animated() const {
  return m_flags & k_animated;
}

inline const vector<loader::vertex>& loader::vertices() const & {
  RX_ASSERT(!is_animated(), "not a static model");
  return as_vertices;
}

inline vector<loader::vertex>&& loader::vertices() && {
  RX_ASSERT(!is_animated(), "not a static model");
  return utility::move(as_vertices);
}

inline const vector<loader::animated_vertex> loader::animated_vertices() const & {
  RX_ASSERT(is_animated(), "not a animated model");
  return as_animated_vertices;
}

inline vector<loader::animated_vertex>&& loader::animated_vertices() && {
  RX_ASSERT(is_animated(), "not a animated model");
  return utility::move(as_animated_vertices);
}

inline const vector<mesh>& loader::meshes() const & {
  return m_meshes;
}

inline const vector<rx_u32>& loader::elements() const & {
  return m_elements;
}

inline vector<rx_u32>&& loader::elements() && {
  return utility::move(m_elements);
}

inline const vector<importer::joint>& loader::joints() const & {
  return m_joints;
}

inline const json& loader::materials() const & {
  return m_materials;
}

} // namespace rx::model

#endif // RX_MODEL_LOADER_H